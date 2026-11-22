#include <array>
#include <cstdint>
#include <cstring>
#include <span>
#include <utility>
#include <vector>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundNodeWavePlayer.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundWave.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
using ue5parse::EUnrealEngineObjectUE4Version;
namespace rt = ue5parse::runtime;

class Writer {
public:
    template <typename T>
    void put(T value) {
        std::size_t at = bytes.size();
        bytes.resize(at + sizeof(T));
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }

    std::vector<std::byte> bytes;
};

class ObjectArchive : public rt::FMemoryReaderView {
public:
    ObjectArchive(std::span<const std::byte> bytes, rt::UObject* object) : FMemoryReaderView(bytes), object(object) {}
    using FMemoryReaderView::operator<<;

    rt::FArchive& operator<<(rt::UObject*& Value) override {
        std::int32_t index = 0;
        *this << index;
        Value = index == 0 ? nullptr : object;
        return *this;
    }

private:
    rt::UObject* object;
};

ue5parse::MountedGame emptyGame(EGame game) {
    ue5parse::MountOptions options;
    options.game = game;
    auto mounted = ue5parse::mount(std::move(options));
    UE5PARSE_CHECK(mounted.has_value());
    return std::move(*mounted);
}

void putObjectStart(Writer& out) {
    out.put<std::uint16_t>(0x0100);
    out.put<std::uint32_t>(0);
}

struct WavePlayerRead {
    bool bError = true;
    bool bAtEnd = false;
    bool bHasSoundWave = false;
};

WavePlayerRead readWavePlayer(EGame game, std::int32_t frameworkVersion, const std::vector<std::byte>& bytes) {
    ue5parse::FileProvider provider(emptyGame(game));
    rt::FRuntime runtime(provider);
    rt::USoundWave* wave = rt::NewObject<rt::USoundWave>(rt::GetTransientPackage());
    rt::USoundNodeWavePlayer* player = rt::NewObject<rt::USoundNodeWavePlayer>(rt::GetTransientPackage());
    const std::array versions{ue5parse::FCustomVersion{rt::FFrameworkObjectVersion::GUID, frameworkVersion}};
    ObjectArchive reader(bytes, wave);
    reader.SetGame(game);
    reader.SetUnversioned(false);
    reader.SetCustomVersions(versions);
    player->Serialize(reader);
    return {reader.IsError(), reader.AtEnd(), player->SoundWave == wave};
}

}

TEST(wavePlayerReadsHardSoundReference) {
    Writer out;
    putObjectStart(out);
    out.put<std::uint16_t>(0x0001);
    out.put<std::int32_t>(1);
    WavePlayerRead read = readWavePlayer(EGame::GAME_UE5_4, rt::FFrameworkObjectVersion::HardSoundReferences, out.bytes);
    CHECK(!read.bError);
    CHECK(read.bAtEnd);
    CHECK(read.bHasSoundWave);
}

TEST(wavePlayerBeforeHardSoundReferencesHasNoReference) {
    Writer out;
    putObjectStart(out);
    out.put<std::uint16_t>(0x0001);
    WavePlayerRead read = readWavePlayer(EGame::GAME_UE4_15, rt::FFrameworkObjectVersion::HardSoundReferences - 1, out.bytes);
    CHECK(!read.bError);
    CHECK(read.bAtEnd);
    CHECK(!read.bHasSoundWave);
}

TEST(soundNodeReadsGraphNodeWithEditorData) {
    Writer out;
    putObjectStart(out);
    out.put<std::uint16_t>(0x0000);
    out.put<std::int32_t>(1);
    out.put<std::int32_t>(1);
    WavePlayerRead read = readWavePlayer(EGame::GAME_UE5_4, rt::FFrameworkObjectVersion::HardSoundReferences, out.bytes);
    CHECK(!read.bError);
    CHECK(read.bAtEnd);
    CHECK(read.bHasSoundWave);
}

TEST(soundNodeBeforeCookedAssetsInEditorHasNoStripFlags) {
    Writer out;
    putObjectStart(out);
    out.put<std::int32_t>(1);
    ue5parse::FileProvider provider(emptyGame(EGame::GAME_UE4_16));
    rt::FRuntime runtime(provider);
    rt::USoundWave* wave = rt::NewObject<rt::USoundWave>(rt::GetTransientPackage());
    rt::USoundNodeWavePlayer* player = rt::NewObject<rt::USoundNodeWavePlayer>(rt::GetTransientPackage());
    const std::array versions{ue5parse::FCustomVersion{rt::FFrameworkObjectVersion::GUID, rt::FFrameworkObjectVersion::HardSoundReferences}};
    ObjectArchive reader(out.bytes, wave);
    reader.SetGame(EGame::GAME_UE4_16);
    reader.SetUEVer(ue5parse::FPackageFileVersion{static_cast<std::int32_t>(EUnrealEngineObjectUE4Version::COOKED_ASSETS_IN_EDITOR_SUPPORT) - 1, 0});
    reader.SetUnversioned(false);
    reader.SetCustomVersions(versions);
    player->Serialize(reader);
    CHECK(!reader.IsError());
    CHECK(reader.AtEnd());
    CHECK(player->SoundWave == wave);
}
