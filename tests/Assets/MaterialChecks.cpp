#include <algorithm>
#include <array>
#include <format>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

#include "AssetSuite.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture.h"
#include "Engine/Source/Runtime/Engine/MaterialShared.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialFunctionInterface.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInstance.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialParameterCollection.h"
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
constexpr std::array dxbcMagic{std::byte{'D'}, std::byte{'X'}, std::byte{'B'}, std::byte{'C'}};

//! The Windows cooks hold D3D shaders in DXBC containers (DXIL too is in one), the Windows-Vulkan cooks hold SPIR-V.
std::span<const std::byte> findShaderMagic(const Cook& cook) {
    return cook.archives.front().parent_path().filename() == "Windows-Vulkan" ? std::span<const std::byte>(spirvMagic) : std::span<const std::byte>(dxbcMagic);
}

void checkMagic(std::span<const std::byte> code, std::span<const std::byte> magic, const std::string& path) {
    if (std::ranges::search(code, magic).empty())
        fail(path, "has shader code without the magic of the cook's shader format");
}

//! Before UE 4.25 the shader library has no shader maps and finds each shader by its output hash.
Result<std::vector<std::byte>> readSharedShader(const Cook& cook, const rt::FShaderCodeLibrary& library, const rt::FMaterialShaderMap& map, const rt::FShader& shader) {
    if (cook.game >= EGame::GAME_UE4_25)
        return library.ReadShaderCode(map.ResourceHash, shader.ResourceIndex);
    if (!map.Code.ShaderHashes.IsValidIndex(shader.ResourceIndex))
        return fail(ErrorCode::InvalidData, "FMaterialShaderMap.ShaderHashes");
    return library.ReadShaderCode(map.Code.ShaderHashes[shader.ResourceIndex]);
}

void checkSharedShaders(const Cook& cook, const Result<void>& opened, const rt::FShaderCodeLibrary& library, const rt::FMaterialShaderMap& map, const std::string& path) {
    if (!opened)
        printError("shader library", opened.error());
    if (!CHECK(opened.has_value()) || (cook.game >= EGame::GAME_UE4_25 && !CHECK(library.ContainsShaderMap(map.ResourceHash))))
        return;
    std::span<const std::byte> magic = findShaderMagic(cook);
    for (const rt::FMeshMaterialShaderMap& mesh : map.Content.OrderedMeshShaderMaps)
        for (const rt::FShader& shader : mesh.Shaders) {
            Result<std::vector<std::byte>> code = readSharedShader(cook, library, map, shader);
            if (!code)
                printError(path, code.error());
            else
                checkMagic(*code, magic, path);
            CHECK(code.has_value());
        }
}

void checkInlineShaders(std::span<const std::byte> magic, const Decompressor* decompressor, const rt::FMaterialShaderMap& map, const std::string& path) {
    if (!CHECK(map.Code.ShaderEntries.Num() > 0) || !CHECK(decompressor != nullptr))
        return;
    std::vector<std::byte> compressed;
    std::vector<std::byte> code;
    for (const rt::FShaderMapResourceCode::FShaderEntry& entry : map.Code.ShaderEntries) {
        std::span<const std::byte> stored = std::as_bytes(std::span(entry.Code.GetData(), static_cast<std::size_t>(entry.Code.Num())));
        if (stored.size() == static_cast<std::size_t>(entry.UncompressedSize)) {
            checkMagic(stored, magic, path);
            continue;
        }
        compressed.assign(stored.begin(), stored.end());
        compressed.resize(stored.size() + decompressor->getInputSlack());
        code.resize(static_cast<std::size_t>(entry.UncompressedSize) + decompressor->getOutputSlack());
        Result<void> decompressed = decompressor->decompress(std::span(compressed).first(stored.size()), std::span(code).first(entry.UncompressedSize));
        if (!decompressed)
            printError(path, decompressed.error());
        else
            checkMagic(std::span(code).first(entry.UncompressedSize), magic, path);
        CHECK(decompressed.has_value());
    }
}

//! UE 4.14 to 4.24 compress shader code with Zlib, UE 4.25 to 4.27 with LZ4, UE5 with Oodle.
const Decompressor* findShaderCodec(const Cook& cook, const FileProvider& provider) {
    if (cook.game >= EGame::GAME_UE5_0)
        return provider.getFiles().getCodecs().find(FName("Oodle"));
    return provider.getFiles().getCodecs().find(FName(cook.game >= EGame::GAME_UE4_25 ? "LZ4" : "Zlib"));
}

void checkMaterialShaders(const Cook& cook, const FileProvider& provider, const Result<void>& opened, const rt::FShaderCodeLibrary& library,
                          const std::string& path) {
    rt::UMaterialInterface* material = loadObject<rt::UMaterialInterface>(path);
    if (material == nullptr)
        return;
    //! A material instance without static parameter overrides uses the shader maps of its parent and stores none.
    const rt::UMaterialInstance* instance = rt::Cast<rt::UMaterialInstance>(material);
    if (instance != nullptr && !instance->bHasStaticPermutationResource)
        return;
    Result<rt::TArray<rt::FMaterialResource>> resources = material->LoadMaterialResources();
    if (!resources)
        printError(path, resources.error());
    if (!CHECK(resources.has_value() && resources->Num() > 0))
        return;
    for (const rt::FMaterialResource& resource : *resources) {
        //! The cook writes bValid false for a material resource whose shader map did not compile, and no shaders.
        if (!resource.ShaderMap)
            continue;
        const rt::FMaterialShaderMap& map = *resource.ShaderMap;
        if (map.bShareCode)
            checkSharedShaders(cook, opened, library, map, path);
        else
            checkInlineShaders(findShaderMagic(cook), findShaderCodec(cook, provider), map, path);
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

void checkFunctions(const rt::TArray<rt::TObjectPtr<rt::UMaterialFunctionInterface>>& functions, std::span<const std::string_view> expected, std::string_view what) {
    if (!checkNear(functions.Num(), static_cast<double>(expected.size()), 0.0, describe(what, "count")))
        return;
    for (std::size_t index = 0; index < expected.size(); index++)
        checkText(getPathName(functions[static_cast<rt::int32>(index)].Get()), expected[index], describe(what, std::to_string(index)));
}

//! tools/RefGen stacks ML_Top over ML_Base with MLB_Ref on M_Layered, and MI_Layered inherits them (RefGenCatalog.cpp).
constexpr std::string_view layeredMaterials[] = {"/Game/Catalog/Materials/M_Layered.M_Layered", "/Game/Catalog/Materials/MI_Layered.MI_Layered"};
constexpr std::string_view expectedLayers[] = {"/Game/Catalog/Materials/ML_Base.ML_Base", "/Game/Catalog/Materials/ML_Top.ML_Top"};
constexpr std::string_view expectedBlends[] = {"/Game/Catalog/Materials/MLB_Ref.MLB_Ref"};

bool isCooked(const Cook& cook, std::string_view path) {
    const Json* assets = cook.expected.find("assets");
    return assets != nullptr && assets->find(path) != nullptr;
}

void checkLayers(const Cook& cook) {
    withRuntime(cook, [&](const FileProvider&) {
        for (std::string_view path : layeredMaterials) {
            if (!isCooked(cook, path))
                continue;
            rt::UMaterialInterface* material = loadObject<rt::UMaterialInterface>(path);
            if (material == nullptr)
                continue;
            rt::FMaterialLayersFunctions layers;
            if (!material->GetMaterialLayers(layers)) {
                fail(path, "has no material layers");
                continue;
            }
            checkFunctions(layers.Layers, expectedLayers, describe(path, "layers"));
            checkFunctions(layers.Blends, expectedBlends, describe(path, "blends"));
        }
    });
}

//! tools/RefGen gives MPC_Ref these two parameters (RefGenMaterials.cpp).
constexpr std::string_view collectionPath = "/Game/Materials/MPC_Ref.MPC_Ref";

void checkCollection(const Cook& cook) {
    withRuntime(cook, [&](const FileProvider&) {
        rt::UMaterialParameterCollection* collection = loadObject<rt::UMaterialParameterCollection>(collectionPath);
        if (collection == nullptr)
            return;
        const rt::FCollectionScalarParameter* wetness = collection->GetScalarParameterByName(rt::FName("GlobalWetness"));
        const rt::FCollectionVectorParameter* tint = collection->GetVectorParameterByName(rt::FName("GlobalTint"));
        if (!CHECK(wetness != nullptr) || !CHECK(tint != nullptr))
            return;
        checkNear(wetness->DefaultValue, 0.25, valueTolerance, describe(collectionPath, "GlobalWetness"));
        const double tintValues[] = {tint->DefaultValue.R, tint->DefaultValue.G, tint->DefaultValue.B, tint->DefaultValue.A};
        const double expectedTint[] = {0.9, 0.95, 1.0, 1.0};
        for (std::size_t channel = 0; channel < std::size(tintValues); channel++)
            checkNear(tintValues[channel], expectedTint[channel], valueTolerance, describe(collectionPath, std::format("GlobalTint.{}", channel)));
    });
}

}

void addMaterialChecks(Checks& checks) {
    checks.cook.emplace("material.collection", checkCollection);
    checks.cook.emplace("material.layers", checkLayers);
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
