#include <cstdint>
#include <cstring>
#include <vector>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/MovieScene/Channels/MovieSceneFloatChannel.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
using ue5parse::runtime::FLightmassPrimitiveSettings;
using ue5parse::runtime::FMemoryReaderView;
using ue5parse::runtime::FMovieSceneFloatChannel;

class Writer {
public:
    template <typename T>
    void put(T value) {
        std::size_t at = bytes.size();
        bytes.resize(at + sizeof(T));
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    void zeros(std::size_t count) { bytes.resize(bytes.size() + count); }

    std::vector<std::byte> bytes;
};

void putFloatChannel(Writer& w, bool bDuplicatedValues) {
    w.put<std::uint8_t>(1);
    w.put<std::uint8_t>(2);
    w.put<std::int32_t>(4);
    w.put<std::int32_t>(1);
    w.put<std::int32_t>(30);
    w.put<std::int32_t>(28);
    w.put<std::int32_t>(1);
    w.put<float>(5.0f);
    w.zeros(24);
    if (bDuplicatedValues) {
        w.put<std::int32_t>(28);
        w.put<std::int32_t>(1);
        w.zeros(28);
    }
    w.put<float>(7.0f);
    w.put<std::uint32_t>(1);
    w.put<std::int32_t>(24000);
    w.put<std::int32_t>(1);
    w.put<std::uint32_t>(0);
}

}

TEST(splitFictionFloatChannelSkipsDuplicatedValuesAndTrailer) {
    Writer w;
    putFloatChannel(w, true);
    w.put<std::uint32_t>(0);
    FMemoryReaderView Ar(w.bytes);
    Ar.SetGame(EGame::GAME_SplitFiction);
    Ar.SetUseUnversionedPropertySerialization(true);
    FMovieSceneFloatChannel Channel;
    REQUIRE(Channel.Serialize(Ar));
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    REQUIRE(Channel.Times.Num() == 1);
    CHECK(Channel.Times[0].Value == 30);
    REQUIRE(Channel.Values.Num() == 1);
    CHECK(Channel.Values[0].Value == 5.0f);
    CHECK(Channel.DefaultValue == 7.0f);
    CHECK(Channel.bHasDefaultValue);
    CHECK(Channel.TickResolution.Numerator == 24000);
}

TEST(stockFloatChannelHasNoDuplicatedValues) {
    Writer w;
    putFloatChannel(w, false);
    FMemoryReaderView Ar(w.bytes);
    Ar.SetGame(EGame::GAME_UE5_4);
    Ar.SetUseUnversionedPropertySerialization(true);
    FMovieSceneFloatChannel Channel;
    REQUIRE(Channel.Serialize(Ar));
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    CHECK(Channel.DefaultValue == 7.0f);
}

TEST(deltaForceFloatChannelUsesTaggedProperties) {
    std::vector<std::byte> bytes(4);
    FMemoryReaderView Ar(bytes);
    Ar.SetGame(EGame::GAME_DeltaForce);
    Ar.SetUseUnversionedPropertySerialization(true);
    FMovieSceneFloatChannel Channel;
    CHECK(!Channel.Serialize(Ar));
    CHECK(Ar.Tell() == 0);
}

TEST(outlastTrialsLightmassSettingsSkipFourBytes) {
    Writer w;
    w.put<std::uint32_t>(1);
    w.put<std::uint32_t>(0);
    w.put<std::uint32_t>(0xDEADBEEF);
    w.put<float>(0.5f);
    w.put<std::uint32_t>(1);
    w.put<std::uint32_t>(0);
    w.put<float>(2.0f);
    w.put<float>(3.0f);
    w.put<float>(4.0f);
    w.put<float>(6.0f);
    FMemoryReaderView Ar(w.bytes);
    Ar.SetGame(EGame::GAME_OutlastTrials);
    FLightmassPrimitiveSettings Settings;
    Ar << Settings;
    CHECK(!Ar.IsError());
    CHECK(Ar.AtEnd());
    CHECK(Settings.bUseTwoSidedLighting);
    CHECK(Settings.FullyOccludedSamplesFraction == 0.5f);
    CHECK(Settings.bUseEmissiveForStaticLighting);
    CHECK(Settings.EmissiveLightFalloffExponent == 2.0f);
    CHECK(Settings.DiffuseBoost == 6.0f);
}
