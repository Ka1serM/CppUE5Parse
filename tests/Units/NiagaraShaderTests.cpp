#include <cstdint>
#include <cstring>
#include <string_view>
#include <utility>
#include <vector>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraScript.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraShared.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
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
    void putBytes(const std::vector<std::byte>& data) { bytes.insert(bytes.end(), data.begin(), data.end()); }
    void putString(std::string_view text) {
        put<std::int32_t>(static_cast<std::int32_t>(text.size() + 1));
        for (char c : text)
            put(c);
        put('\0');
    }
    void putHash(std::uint8_t first) {
        put(first);
        for (int index = 1; index < 20; index++)
            put<std::uint8_t>(0);
    }
    void align(std::size_t alignment) {
        while (bytes.size() % alignment != 0)
            put<std::uint8_t>(0);
    }
    std::uint32_t tell() const { return static_cast<std::uint32_t>(bytes.size()); }
    std::uint32_t arrayHeader(std::int32_t num) {
        align(8);
        std::uint32_t field = tell();
        put<std::uint64_t>(0);
        put(num);
        put(num);
        return field;
    }
    void pointTo(std::uint32_t field, std::uint32_t target, EGame game) {
        const std::uint32_t shift = game < EGame::GAME_UE5_0 ? 1u : 24u;
        std::uint64_t packed = (static_cast<std::uint64_t>(target - field) << shift) | 1u;
        std::memcpy(bytes.data() + field, &packed, sizeof(packed));
    }

    std::vector<std::byte> bytes;
};

class NameArchive : public rt::FMemoryReaderView {
public:
    using rt::FMemoryReaderView::FMemoryReaderView;
    using rt::FMemoryReaderView::operator<<;

    rt::FArchive& operator<<(rt::FName& Value) override {
        rt::FString Text;
        *this << Text;
        Value = rt::FName(Text.ToUtf8());
        return *this;
    }
};

ue5parse::MountedGame emptyGame(EGame game) {
    ue5parse::MountOptions options;
    options.game = game;
    auto mounted = ue5parse::mount(std::move(options));
    UE5PARSE_CHECK(mounted.has_value());
    return std::move(*mounted);
}

bool hasVariablesAndDependencies(EGame game) {
    return game >= EGame::GAME_UE4_27 || game == EGame::GAME_HonorofKingsWorld;
}

std::vector<std::byte> frozenContent(EGame game) {
    Writer out;
    out.put<std::uint64_t>(0);
    out.put<std::uint64_t>(0);
    out.put<std::uint32_t>(0);
    out.put<std::uint32_t>(0);
    for (int index = 0; index < 4; index++)
        out.arrayHeader(0);
    if (game < EGame::GAME_UE5_2) {
        out.put<std::uint8_t>(0);
        if (game == EGame::GAME_HonorofKingsWorld) {
            out.align(8);
            for (int index = 0; index < 152; index++)
                out.put<std::uint8_t>(0);
        }
    } else {
        out.put<std::uint64_t>(0);
    }
    std::uint32_t friendlyName = 0;
    if (game < EGame::GAME_UE5_4) {
        friendlyName = out.arrayHeader(3);
        out.arrayHeader(0);
    }
    out.align(8);
    for (std::uint32_t part : {1u, 2u, 3u, 4u})
        out.put(part);
    out.put<std::int32_t>(3);
    if (game < EGame::GAME_UE5_6)
        out.bytes.resize(out.bytes.size() + 16);
    std::uint32_t defines = out.arrayHeader(1);
    if (hasVariablesAndDependencies(game))
        out.arrayHeader(0);
    out.putHash(0xab);
    std::uint32_t referenced = out.arrayHeader(1);
    out.put<std::uint32_t>(8);
    out.put<std::uint32_t>(1);
    std::uint32_t dependencies = hasVariablesAndDependencies(game) ? out.arrayHeader(1) : 0u;
    out.put<std::uint8_t>(0);
    out.align(8);

    if (friendlyName != 0u) {
        out.align(2);
        out.pointTo(friendlyName, out.tell(), game);
        for (char16_t c : std::u16string_view(u"FX\0", 3))
            out.put(c);
    }
    out.align(8);
    out.pointTo(defines, out.tell(), game);
    std::uint32_t define = out.arrayHeader(4);
    out.pointTo(define, out.tell(), game);
    for (char16_t c : std::u16string_view(u"A=1\0", 4))
        out.put(c);
    out.pointTo(referenced, out.tell(), game);
    out.putHash(0xcd);
    if (dependencies != 0u) {
        out.align(8);
        out.pointTo(dependencies, out.tell(), game);
        out.put<std::uint64_t>(0x1122334455667788ull);
        out.put<std::int32_t>(5);
        out.putHash(0xef);
        out.align(8);
    }
    return out.bytes;
}

void writeShaderScript(Writer& out, EGame game) {
    const bool bUE4 = game < EGame::GAME_UE5_0;
    out.put<std::uint32_t>(1);
    if (game >= EGame::GAME_UE4_26)
        out.put<std::int32_t>(2);
    if (game >= EGame::GAME_UE4_26 && game < EGame::GAME_UE5_0) {
        out.put<std::int32_t>(1);
        out.put<std::int32_t>(0);
        out.put<std::int32_t>(1);
    }
    if (game >= EGame::GAME_UE5_2 || game == EGame::GAME_DuetNightAbyss || game == EGame::GAME_HonorofKingsWorld) {
        out.put<std::int32_t>(2);
        out.put<std::uint8_t>(0x12);
        out.put<std::uint8_t>(0x34);
    }
    out.put<std::uint32_t>(1);
    if (!bUE4) {
        out.put<std::uint32_t>(8);
        out.put<std::uint32_t>(1);
    }
    std::vector<std::byte> frozen = frozenContent(game);
    out.put(static_cast<std::uint32_t>(frozen.size()));
    out.putBytes(frozen);
    auto writePointerTable = [&] {
        if (!bUE4)
            out.put<std::int32_t>(0);
        out.put<std::int32_t>(0);
        out.put<std::int32_t>(0);
        out.put<std::int32_t>(1);
        out.putString("NiagaraDataInterfaceCurve");
    };
    if (!bUE4)
        writePointerTable();
    out.put<std::uint32_t>(0);
    out.put<std::uint32_t>(0);
    out.put<std::uint32_t>(0);
    if (bUE4) {
        writePointerTable();
        out.put<std::int32_t>(0);
    }
    out.put<std::uint32_t>(1);
    if (game >= EGame::GAME_UE5_2)
        out.putString("PCD3D_SM6");
    else if (game >= EGame::GAME_UE5_0)
        out.put<std::uint8_t>(0);
    out.putHash(0x77);
}

void checkShaderScript(const rt::FNiagaraShaderScript& script, EGame game) {
    CHECK(script.bLoadedFromCookedMaterial);
    REQUIRE(script.GameThreadShaderMap.has_value());
    const rt::FNiagaraShaderMap& map = *script.GameThreadShaderMap;
    REQUIRE(map.PointerTable.DITypes.Num() == 1);
    CHECK(map.PointerTable.DITypes[0].ToUtf8() == "NiagaraDataInterfaceCurve");
    CHECK(map.bShareCode);
    const rt::FNiagaraShaderMapId& id = map.Content.ShaderMapId;
    CHECK(id.FeatureLevel == rt::ERHIFeatureLevel::SM5);
    CHECK(id.BaseCompileHash.Hash[0] == 0xab);
    REQUIRE(id.AdditionalDefines.Num() == 1);
    CHECK(id.AdditionalDefines[0].ToUtf8() == "A=1");
    REQUIRE(id.ReferencedCompileHashes.Num() == 1);
    CHECK(id.ReferencedCompileHashes[0].Hash[0] == 0xcd);
    CHECK(id.LayoutParams.MaxFieldAlignment == 8u);
    CHECK(!id.bUsesRapidIterationParams);
    if (game < EGame::GAME_UE5_4)
        CHECK(map.Content.FriendlyName.ToUtf8() == "FX");
    if (!hasVariablesAndDependencies(game)) {
        CHECK(id.ShaderTypeDependencies.Num() == 0);
        return;
    }
    REQUIRE(id.ShaderTypeDependencies.Num() == 1);
    CHECK(id.ShaderTypeDependencies[0].ShaderTypeName.GetHash() == 0x1122334455667788ull);
    CHECK(id.ShaderTypeDependencies[0].PermutationId == 5);
    CHECK(id.ShaderTypeDependencies[0].SourceHash.Hash[0] == 0xef);
}

rt::FNiagaraShaderScript readShaderScript(EGame game) {
    Writer out;
    writeShaderScript(out, game);
    NameArchive reader(out.bytes);
    reader.SetGame(game);
    rt::FNiagaraShaderScript script;
    script.SerializeShaderMap(reader);
    CHECK(!reader.IsError());
    CHECK(reader.AtEnd());
    return script;
}

}

TEST(niagaraScriptReadsCookedShaderMapsUE5_4) {
    Writer out;
    out.put<std::int32_t>(1);
    writeShaderScript(out, EGame::GAME_UE5_4);
    ue5parse::FileProvider provider(emptyGame(EGame::GAME_UE5_4));
    rt::FRuntime runtime(provider);
    NameArchive reader(out.bytes);
    reader.SetGame(EGame::GAME_UE5_4);
    rt::UNiagaraScript* script = rt::NewObject<rt::UNiagaraScript>(rt::GetTransientPackage());
    script->SerializeNiagaraShaderMaps(reader);
    CHECK(!reader.IsError());
    CHECK(reader.AtEnd());
    REQUIRE(script->LoadedScriptResources.Num() == 1);
    const rt::FNiagaraShaderScript& resource = script->LoadedScriptResources[0];
    CHECK(resource.NumPermutations == 2);
    CHECK(resource.BaseCompileHash.DataHash.Num() == 2);
    REQUIRE(resource.GameThreadShaderMap.has_value());
    CHECK(resource.GameThreadShaderMap->ShaderPlatformName == rt::FName("PCD3D_SM6"));
    checkShaderScript(resource, EGame::GAME_UE5_4);
}

TEST(niagaraShaderScriptReadsUE4_26ShaderStages) {
    rt::FNiagaraShaderScript script = readShaderScript(EGame::GAME_UE4_26);
    REQUIRE(script.ShaderStageToPermutation.Num() == 1);
    CHECK(script.ShaderStageToPermutation[0].Value == 1);
    CHECK(script.BaseCompileHash.DataHash.Num() == 0);
    checkShaderScript(script, EGame::GAME_UE4_26);
}

TEST(niagaraShaderScriptReadsDuetNightAbyssBaseCompileHash) {
    rt::FNiagaraShaderScript script = readShaderScript(EGame::GAME_DuetNightAbyss);
    REQUIRE(script.BaseCompileHash.DataHash.Num() == 2);
    CHECK(script.BaseCompileHash.DataHash[1] == 0x34);
    checkShaderScript(script, EGame::GAME_DuetNightAbyss);
}

TEST(niagaraShaderScriptReadsHonorOfKingsWorldShaderMapId) {
    rt::FNiagaraShaderScript script = readShaderScript(EGame::GAME_HonorofKingsWorld);
    CHECK(script.ShaderStageToPermutation.Num() == 1);
    CHECK(script.BaseCompileHash.DataHash.Num() == 2);
    checkShaderScript(script, EGame::GAME_HonorofKingsWorld);
}
