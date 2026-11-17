#include <algorithm>
#include <array>
#include <string>
#include <type_traits>
#include <vector>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture.h"
#include "Engine/Source/Runtime/Engine/MaterialShared.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInstance.h"
#include "Engine/Source/Runtime/RenderCore/ShaderCodeArchive.h"

namespace test::assets {

using namespace ue5parse;

namespace {

template <class Value, class Compare>
void checkParameters(const std::string& path, const Json& value, Compare compare) {
    rt::UMaterialInterface* material = loadObject<rt::UMaterialInterface>(path);
    if (material == nullptr)
        return;
    for (const auto& [name, expected] : value.getObject()) {
        Value actual{};
        if (!compare(*material, rt::FMaterialParameterInfo(rt::FName(name)), actual))
            fail(describe(path, name), "no such parameter");
        else if constexpr (std::is_same_v<Value, float>)
            checkNear(actual, expected.getNumber(), valueTolerance, describe(path, name));
        else if constexpr (std::is_same_v<Value, rt::FLinearColor>)
            checkColor(actual, expected, describe(path, name));
        else if constexpr (std::is_same_v<Value, bool>)
            checkText(actual ? "true" : "false", expected.getBool() ? "true" : "false", describe(path, name));
        else
            checkText(getPathName(actual), expected.getString(), describe(path, name));
    }
}

void checkScalars(const std::string& path, const Json& value) {
    checkParameters<float>(path, value, [](const rt::UMaterialInterface& material, const rt::FMaterialParameterInfo& info, float& out) {
        return material.GetScalarParameterValue(info, out);
    });
}

void checkVectors(const std::string& path, const Json& value) {
    checkParameters<rt::FLinearColor>(path, value, [](const rt::UMaterialInterface& material, const rt::FMaterialParameterInfo& info, rt::FLinearColor& out) {
        return material.GetVectorParameterValue(info, out);
    });
}

void checkTextures(const std::string& path, const Json& value) {
    checkParameters<rt::UTexture*>(path, value, [](const rt::UMaterialInterface& material, const rt::FMaterialParameterInfo& info, rt::UTexture*& out) {
        return material.GetTextureParameterValue(info, out);
    });
}

void checkSwitches(const std::string& path, const Json& value) {
    checkParameters<bool>(path, value, [](const rt::UMaterialInterface& material, const rt::FMaterialParameterInfo& info, bool& out) {
        return material.GetStaticSwitchParameterValue(info, out);
    });
}

void checkParent(const std::string& path, const Json& value) {
    rt::UMaterialInstance* instance = loadObject<rt::UMaterialInstance>(path);
    if (instance != nullptr)
        checkText(getPathName(instance->Parent), value.getString(), describe(path, "parent"));
}

void checkBlendMode(const std::string& path, const Json& value) {
    checkReflectedProperty(path, "BlendMode", value);
}

void checkDomain(const std::string& path, const Json& value) {
    checkReflectedProperty(path, "MaterialDomain", value);
}

constexpr std::array spirvMagic{std::byte{0x03}, std::byte{0x02}, std::byte{0x23}, std::byte{0x07}};

void checkSpirv(std::span<const std::byte> code, const std::string& path) {
    if (std::ranges::search(code, spirvMagic).empty())
        fail(path, "has shader code without the SPIR-V magic");
}

void checkSharedShaders(const Result<void>& opened, const rt::FShaderCodeLibrary& library, const rt::FMaterialShaderMap& map, const std::string& path) {
    if (!opened)
        printError("shader library", opened.error());
    if (!CHECK(opened.has_value()) || !CHECK(library.ContainsShaderMap(map.ResourceHash)))
        return;
    for (const rt::FMeshMaterialShaderMap& mesh : map.Content.OrderedMeshShaderMaps)
        for (const rt::FShader& shader : mesh.Shaders) {
            Result<std::vector<std::byte>> code = library.ReadShaderCode(map.ResourceHash, shader.ResourceIndex);
            if (!code)
                printError(path, code.error());
            else
                checkSpirv(*code, path);
            CHECK(code.has_value());
        }
}

void checkInlineShaders(const Decompressor* decompressor, const rt::FMaterialShaderMap& map, const std::string& path) {
    if (!CHECK(map.Code.ShaderEntries.Num() > 0) || !CHECK(decompressor != nullptr))
        return;
    std::vector<std::byte> compressed;
    std::vector<std::byte> code;
    for (const rt::FShaderMapResourceCode::FShaderEntry& entry : map.Code.ShaderEntries) {
        std::span<const std::byte> stored = std::as_bytes(std::span(entry.Code.GetData(), static_cast<std::size_t>(entry.Code.Num())));
        if (stored.size() == static_cast<std::size_t>(entry.UncompressedSize)) {
            checkSpirv(stored, path);
            continue;
        }
        compressed.assign(stored.begin(), stored.end());
        compressed.resize(stored.size() + decompressor->getInputSlack());
        code.resize(static_cast<std::size_t>(entry.UncompressedSize) + decompressor->getOutputSlack());
        Result<void> decompressed = decompressor->decompress(std::span(compressed).first(stored.size()), std::span(code).first(entry.UncompressedSize));
        if (!decompressed)
            printError(path, decompressed.error());
        else
            checkSpirv(std::span(code).first(entry.UncompressedSize), path);
        CHECK(decompressed.has_value());
    }
}

//! UE4 compresses shader code with LZ4, UE5 with Oodle.
const Decompressor* findShaderCodec(const Cook& cook, const FileProvider& provider) {
    return provider.getFiles().getCodecs().find(FName(cook.game < EGame::GAME_UE5_0 ? "LZ4" : "Oodle"));
}

void checkMaterialShaders(const Cook& cook, const FileProvider& provider, const Result<void>& opened, const rt::FShaderCodeLibrary& library,
                          const std::string& path) {
    rt::UMaterialInterface* material = loadObject<rt::UMaterialInterface>(path);
    if (material == nullptr)
        return;
    Result<rt::TArray<rt::FMaterialResource>> resources = material->LoadMaterialResources();
    if (!resources)
        printError(path, resources.error());
    if (!CHECK(resources.has_value() && resources->Num() > 0))
        return;
    for (const rt::FMaterialResource& resource : *resources) {
        if (!CHECK(resource.ShaderMap.has_value()))
            continue;
        const rt::FMaterialShaderMap& map = *resource.ShaderMap;
        if (map.bShareCode)
            checkSharedShaders(opened, library, map, path);
        else
            checkInlineShaders(findShaderCodec(cook, provider), map, path);
    }
}

//! Some engines' Python API can't read blend modes, so their reference files have no material.blend values.
std::vector<std::string> findMaterials(const Cook& cook) {
    std::vector<std::string> paths;
    const Json* values = cook.expected.find("values");
    if (const Json* blend = values == nullptr ? nullptr : values->find("material.blend")) {
        for (const auto& [path, blendMode] : blend->getObject())
            paths.push_back(path);
        return paths;
    }
    if (const Json* assets = cook.expected.find("assets"))
        for (const auto& [path, assetClass] : assets->getObject())
            if (assetClass.getString() == "Material" || assetClass.getString() == "MaterialInstanceConstant")
                paths.push_back(path);
    return paths;
}

void checkShaders(const Cook& cook) {
    std::vector<std::string> materials = findMaterials(cook);
    if (materials.empty()) {
        fail(cook.version, "the reference values list no materials");
        return;
    }
    withRuntime(cook, [&](const FileProvider& provider) {
        rt::FShaderCodeLibrary library(provider);
        Result<void> opened = library.OpenLibraries();
        for (const std::string& path : materials)
            checkMaterialShaders(cook, provider, opened, library, path);
    });
}

}

void addMaterialChecks(Checks& checks) {
    checks.cook.emplace("shaders", checkShaders);
    checks.values.emplace("material.blend", checkBlendMode);
    checks.values.emplace("material.domain", checkDomain);
    checks.values.emplace("material.parent", checkParent);
    checks.values.emplace("material.scalar", checkScalars);
    checks.values.emplace("material.switch", checkSwitches);
    checks.values.emplace("material.texture", checkTextures);
    checks.values.emplace("material.vector", checkVectors);
}

}
