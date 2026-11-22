#include <cstdint>
#include <cstring>
#include <functional>
#include <utility>
#include <vector>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraSpriteRendererProperties.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/Engine/Particles/SubUVAnimation.h"
#include "Test.h"

namespace {

using ue5parse::EGame;
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

ue5parse::MountedGame emptyGame(EGame game) {
    ue5parse::MountOptions options;
    options.game = game;
    auto mounted = ue5parse::mount(std::move(options));
    UE5PARSE_CHECK(mounted.has_value());
    return std::move(*mounted);
}

void writeObjectStart(Writer& out) {
    out.put<std::uint16_t>(0x0100);
    out.put<std::uint32_t>(0);
}

void writeBoundingGeometry(Writer& out) {
    out.put<std::int32_t>(2);
    for (float value : {0.25f, 0.5f, 0.75f, 1.0f})
        out.put(value);
}

template <class T>
void serialize(EGame game, const std::vector<std::byte>& bytes, const std::function<void(T&, const rt::FMemoryReaderView&)>& check) {
    ue5parse::FileProvider provider(emptyGame(game));
    rt::FRuntime runtime(provider);
    rt::FMemoryReaderView reader(bytes);
    reader.SetGame(game);
    T* object = rt::NewObject<T>(rt::GetTransientPackage());
    object->Serialize(reader);
    check(*object, reader);
}

}

TEST(niagaraSpriteRendererReadsCookedCutoutGeometry) {
    Writer out;
    writeObjectStart(out);
    writeBoundingGeometry(out);
    serialize<rt::UNiagaraSpriteRendererProperties>(EGame::GAME_UE5_4, out.bytes,
                                                    [](rt::UNiagaraSpriteRendererProperties& renderer, const rt::FMemoryReaderView& reader) {
                                                        CHECK(!reader.IsError());
                                                        CHECK(reader.AtEnd());
                                                        REQUIRE(renderer.DerivedData.BoundingGeometry.Num() == 2);
                                                        CHECK(renderer.DerivedData.BoundingGeometry[1].X == 0.75f);
                                                        CHECK(renderer.DerivedData.BoundingGeometry[1].Y == 1.0f);
                                                    });
}

TEST(niagaraSpriteRendererBeforeUE4_23HasNoCutoutGeometry) {
    Writer out;
    writeObjectStart(out);
    serialize<rt::UNiagaraSpriteRendererProperties>(EGame::GAME_UE4_22, out.bytes,
                                                    [](rt::UNiagaraSpriteRendererProperties& renderer, const rt::FMemoryReaderView& reader) {
                                                        CHECK(!reader.IsError());
                                                        CHECK(reader.AtEnd());
                                                        CHECK(renderer.DerivedData.BoundingGeometry.Num() == 0);
                                                    });
}

TEST(subUVAnimationReadsCookedDerivedData) {
    Writer out;
    writeObjectStart(out);
    out.put<std::uint32_t>(1);
    writeBoundingGeometry(out);
    serialize<rt::USubUVAnimation>(EGame::GAME_UE5_4, out.bytes, [](rt::USubUVAnimation& animation, const rt::FMemoryReaderView& reader) {
        CHECK(!reader.IsError());
        CHECK(reader.AtEnd());
        CHECK(animation.IsBoundingGeometryValid());
        CHECK(animation.DerivedData.BoundingGeometry.Num() == 2);
    });
}
