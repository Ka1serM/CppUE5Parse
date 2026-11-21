#include "Conversion/Materials/MaterialShaderExport.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <string_view>
#include <utility>

#include "Conversion/Shaders/EngineShaderParameterLayouts.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"
#include "Engine/Source/Runtime/Engine/Materials/Material.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionMaterialAttributeLayers.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionRuntimeVirtualTextureSample.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialExpressionTextureBase.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialFunction.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialInstance.h"
#include "Engine/Source/Runtime/Engine/Materials/MaterialParameterCollection.h"

namespace ue5parse::conversion {

using namespace runtime;

namespace {

constexpr uint8 samplerSourceFromTextureAsset = 0u;
constexpr uint8 samplerSourceWrapWorldGroupSettings = 1u;
constexpr uint8 samplerSourceClampWorldGroupSettings = 2u;

std::string nameString(const FName& name) {
    return name.isNone() ? std::string() : name.toString();
}

//! The light map policies of the base pass shaders, from IMPLEMENT_BASEPASS_LIGHTMAPPED_SHADER_TYPE in BasePassRendering.cpp of UE 4.0 to 5.9.
constexpr std::string_view baseShaderPolicies[] = {
    "FNoLightMapPolicy",
    "FSelfShadowedTranslucencyPolicy",
    "FSelfShadowedCachedPointIndirectLightingPolicy",
    "FSelfShadowedVolumetricLightmapPolicy",
    "FPrecomputedVolumetricLightmapLightingPolicy",
    "FCachedVolumeIndirectLightingPolicy",
    "FCachedPointIndirectLightingPolicy",
    "FSimpleNoLightmapLightingPolicy",
    "FSimpleLightmapOnlyLightingPolicy",
    "FSimpleDirectionalLightLightingPolicy",
    "FSimpleDynamicLightingPolicy",
    "FSimpleStationaryLightPrecomputedShadowsLightingPolicy",
    "FSimpleStationaryLightSingleSampleShadowsLightingPolicy",
    "FSimpleStationaryLightVolumetricLightmapShadowsLightingPolicy",
    "TLightMapPolicyLQ",
    "TLightMapPolicyHQ",
    "TDistanceFieldShadowsAndLightMapPolicyHQ",
};

//! The base pass shader type names: TBasePassVS, TBasePassVS with AtmosphericFog, TBasePassPS with and without Skylight.
std::vector<std::string> getBaseShaderNames() {
    std::vector<std::string> names;
    for (std::string_view policy : baseShaderPolicies) {
        names.push_back("TBasePassVS" + std::string(policy));
        names.push_back("TBasePassVS" + std::string(policy) + "AtmosphericFog");
        names.push_back("TBasePassPS" + std::string(policy));
        names.push_back("TBasePassPS" + std::string(policy) + "Skylight");
    }
    return names;
}

class ShaderTypeNames {
public:
    //! UE4, UE 5.0 and UE 5.1 cooks keep only the hashes of the shader type names, so the base pass shader names are hashed to find them.
    explicit ShaderTypeNames(const FShaderMapPointerTable& table) {
        for (const FPointerTableBase::FTypeDependency& dependency : table.GetTypeDependencies())
            add(nameString(dependency.Name));
        for (std::string& name : getBaseShaderNames())
            add(std::move(name));
        std::ranges::sort(names);
        const auto [first, last] = std::ranges::unique(names);
        names.erase(first, last);
    }

    std::string_view find(FHashedName name) const {
        const auto found = std::ranges::lower_bound(names, name.GetHash(), {}, &std::pair<uint64, std::string>::first);
        return found != names.end() && found->first == name.GetHash() ? std::string_view(found->second) : std::string_view();
    }

private:
    void add(std::string name) {
        if (!name.empty())
            names.emplace_back(FHashedName(std::string_view(name)).GetHash(), std::move(name));
    }

    std::vector<std::pair<uint64, std::string>> names;
};

FMaterialParameterInfo toParameterInfo(const FHashedMaterialParameterInfo& info) {
    return {info.Name, info.Association, info.Index};
}

//! Before UE 4.25 the cook keeps no list of the textures a material references; UE rebuilds it from the material expressions as AppendReferencedTextures does.
class LegacyTextureReferences {
public:
    explicit LegacyTextureReferences(EGame game) : game(game) {}

    void appendInstance(const UMaterialInstance& instance) {
        if (game >= EGame::GAME_UE4_19)
            for (const FStaticMaterialLayersParameter& parameter : instance.StaticParameters.MaterialLayersParameters) {
                if (!parameter.bOverride)
                    continue;
                appendLayerFunctions(parameter.Value.Layers);
                appendLayerFunctions(parameter.Value.Blends);
            }
        if (const auto* parent = Cast<UMaterialInstance>(instance.Parent.Get()))
            appendInstance(*parent);
        else if (const auto* material = Cast<UMaterial>(instance.Parent.Get()))
            appendMaterial(*material);
    }

    void appendMaterial(const UMaterial& material) {
        for (const TObjectPtr<UMaterialExpression>& expression : material.Expressions) {
            const auto* call = Cast<UMaterialExpressionMaterialFunctionCall>(expression.Get());
            if (call != nullptr && call->MaterialFunction) {
                for (const UMaterialFunctionInterface* function : getDependentFunctions(*call->MaterialFunction))
                    appendFunction(*function);
                continue;
            }
            const auto* layers = Cast<UMaterialExpressionMaterialAttributeLayers>(expression.Get());
            if (layers != nullptr && game >= EGame::GAME_UE4_19) {
                std::vector<const UMaterialFunctionInterface*> functions;
                addLayerFunctions(layers->DefaultLayers.Layers, functions);
                addLayerFunctions(layers->DefaultLayers.Blends, functions);
                for (const UMaterialFunctionInterface* function : functions)
                    appendFunction(*function);
                continue;
            }
            if (expression)
                appendExpression(*expression, game >= EGame::GAME_UE4_23);
        }
    }

    const std::vector<const UTexture*>& getTextures() const { return textures; }

private:
    //! Before UE 4.19 the function itself comes first and its nested functions after it.
    std::vector<const UMaterialFunctionInterface*> getDependentFunctions(const UMaterialFunctionInterface& function) const {
        std::vector<const UMaterialFunctionInterface*> functions;
        if (game < EGame::GAME_UE4_19)
            functions.push_back(&function);
        addNestedFunctions(function, functions);
        if (game >= EGame::GAME_UE4_19)
            addUnique(functions, &function);
        return functions;
    }

    static void addNestedFunctions(const UMaterialFunctionInterface& function, std::vector<const UMaterialFunctionInterface*>& functions) {
        const auto* materialFunction = Cast<UMaterialFunction>(&function);
        if (materialFunction == nullptr)
            return;
        for (const TObjectPtr<UMaterialExpression>& expression : materialFunction->FunctionExpressions) {
            const auto* call = Cast<UMaterialExpressionMaterialFunctionCall>(expression.Get());
            if (call == nullptr || !call->MaterialFunction)
                continue;
            addNestedFunctions(*call->MaterialFunction, functions);
            addUnique(functions, call->MaterialFunction.Get());
        }
    }

    static void addLayerFunctions(const TArray<TObjectPtr<UMaterialFunctionInterface>>& layers, std::vector<const UMaterialFunctionInterface*>& functions) {
        for (const TObjectPtr<UMaterialFunctionInterface>& layer : layers) {
            if (!layer)
                continue;
            addNestedFunctions(*layer, functions);
            addUnique(functions, layer.Get());
        }
    }

    void appendLayerFunctions(const TArray<TObjectPtr<UMaterialFunctionInterface>>& layers) {
        for (const TObjectPtr<UMaterialFunctionInterface>& layer : layers)
            if (layer)
                appendFunction(*layer);
    }

    void appendFunction(const UMaterialFunctionInterface& function) {
        const auto* materialFunction = Cast<UMaterialFunction>(&function);
        if (materialFunction == nullptr)
            return;
        for (const TObjectPtr<UMaterialExpression>& expression : materialFunction->FunctionExpressions)
            if (expression)
                appendExpression(*expression, false);
    }

    //! UE 4.20 keeps a slot for each expression that can reference a texture, even when the cook stripped the texture; before UE 4.23 it adds the slot without merging duplicates.
    void appendExpression(const UMaterialExpression& expression, bool unique) {
        const auto* textureExpression = Cast<UMaterialExpressionTextureBase>(&expression);
        if (textureExpression == nullptr && !expression.IsA<UMaterialExpressionRuntimeVirtualTextureSample>())
            return;
        //! A runtime virtual texture keeps its slot, but it is no UTexture that a texture slot can take.
        const UTexture* texture = textureExpression != nullptr ? textureExpression->Texture.Get() : nullptr;
        if (game < EGame::GAME_UE4_20) {
            if (texture != nullptr)
                addUnique(textures, texture);
            return;
        }
        if (unique)
            addUnique(textures, texture);
        else
            textures.push_back(texture);
    }

    template <class T>
    static void addUnique(std::vector<const T*>& items, const T* item) {
        if (!std::ranges::contains(items, item))
            items.push_back(item);
    }

    EGame game;
    std::vector<const UTexture*> textures;
};

std::vector<const UTexture*> getReferencedTextures(const UMaterialInterface& owner) {
    std::vector<const UTexture*> result;
    if (const EGame game = GetRuntimeProvider().getGame(); game < EGame::GAME_UE4_25) {
        LegacyTextureReferences references(game);
        if (const auto* instance = Cast<UMaterialInstance>(&owner))
            references.appendInstance(*instance);
        else if (const auto* material = Cast<UMaterial>(&owner))
            references.appendMaterial(*material);
        return references.getTextures();
    }
    const FMaterialCachedExpressionData* data = owner.GetCachedExpressionData();
    if (data == nullptr)
        return result;
    result.reserve(static_cast<std::size_t>(data->ReferencedTextures.Num()));
    for (const TObjectPtr<UObject>& object : data->ReferencedTextures)
        result.push_back(Cast<UTexture>(object.Get()));
    return result;
}

std::vector<const UMaterialParameterCollection*> getParameterCollections(const UMaterialInterface& owner, const TArray<FGuid>& ids) {
    std::vector<const UMaterialParameterCollection*> result(static_cast<std::size_t>(ids.Num()));
    const FMaterialCachedExpressionData* data = owner.GetCachedExpressionData();
    if (data == nullptr)
        return result;
    for (const FMaterialParameterCollectionInfo& info : data->ParameterCollectionInfos) {
        if (info.ParameterCollection == nullptr)
            continue;
        for (int32 index = 0; index < ids.Num(); ++index)
            if (ids[index] == info.StateId)
                result[static_cast<std::size_t>(index)] = info.ParameterCollection.Get();
    }
    return result;
}

TextureAddress getAddress(uint8 samplerSource, const UTexture* texture, bool vertical) {
    if (samplerSource == samplerSourceWrapWorldGroupSettings)
        return TA_Wrap;
    if (samplerSource == samplerSourceClampWorldGroupSettings)
        return TA_Clamp;
    const auto* texture2D = Cast<UTexture2D>(texture);
    if (texture2D == nullptr)
        return TA_Wrap;
    return vertical ? texture2D->AddressY.GetValue() : texture2D->AddressX.GetValue();
}

bool isNearest(uint8 samplerSource, const UTexture* texture) {
    return samplerSource == samplerSourceFromTextureAsset && texture != nullptr && texture->Filter == TF_Nearest;
}

std::array<int32, 3> getTextureSize(const UTexture* texture) {
    if (texture == nullptr)
        return {};
    if (const FTexturePlatformData* data = texture->GetPlatformData())
        return {data->SizeX, data->SizeY, data->GetNumSlices()};
    int32 width = 0;
    int32 height = 0;
    if (!GetPropertyValue(texture, FName("SizeX"), width) || !GetPropertyValue(texture, FName("SizeY"), height))
        return {};
    return {std::max(width, 1), std::max(height, 1), 1};
}

Result<MaterialNumericParameterValue> getNumericParameter(const UMaterialInterface& material, const FMaterialNumericParameterInfo& numeric,
                                                          std::span<const uint8> defaults) {
    MaterialNumericParameterValue parameter{nameString(numeric.ParameterInfo.Name), numeric.ParameterType, {0.0f, 0.0f, 0.0f, 0.0f}};
    const FMaterialParameterInfo info = toParameterInfo(numeric.ParameterInfo);
    if (numeric.ParameterType == EMaterialParameterType::Scalar) {
        float value = 0.0f;
        if (!material.GetScalarParameterValue(info, value, true)) {
            if (numeric.DefaultValueOffset + sizeof(float) > defaults.size())
                return fail(ErrorCode::InvalidData, "ExportMaterialShaders.DefaultValueOffset", numeric.DefaultValueOffset);
            std::memcpy(&value, defaults.data() + numeric.DefaultValueOffset, sizeof(float));
        }
        parameter.value.R = value;
        return parameter;
    }
    if (numeric.ParameterType != EMaterialParameterType::Vector)
        return fail(ErrorCode::InvalidData, "ExportMaterialShaders.ParameterType", static_cast<uint64>(numeric.ParameterType));
    if (!material.GetVectorParameterValue(info, parameter.value, true)) {
        if (numeric.DefaultValueOffset + sizeof(FLinearColor) > defaults.size())
            return fail(ErrorCode::InvalidData, "ExportMaterialShaders.DefaultValueOffset", numeric.DefaultValueOffset);
        std::memcpy(&parameter.value, defaults.data() + numeric.DefaultValueOffset, sizeof(FLinearColor));
    }
    return parameter;
}

std::vector<MaterialTextureSlot> getTextureSlots(const UMaterialInterface& material, const FUniformExpressionSet& expressions,
                                                 std::span<const UTexture* const> referenced) {
    std::vector<MaterialTextureSlot> slots;
    for (uint32 type = 0u; type < NumMaterialTextureParameterTypes; ++type) {
        const TArray<FMaterialTextureParameterInfo>& infos = expressions.UniformTextureParameters[type];
        for (int32 index = 0; index < infos.Num(); ++index) {
            const FMaterialTextureParameterInfo& info = infos[index];
            std::string name = nameString(info.ParameterInfo.Name);
            UTexture* parameter = nullptr;
            const UTexture* texture =
                !name.empty() && material.GetTextureParameterValue(toParameterInfo(info.ParameterInfo), parameter) && parameter ? parameter
                : info.TextureIndex >= 0 && static_cast<std::size_t>(info.TextureIndex) < referenced.size()                     ? referenced[info.TextureIndex]
                                                                                                                                : nullptr;
            slots.push_back({std::move(name), texture, static_cast<EMaterialTextureParameterType>(type), index, getAddress(info.SamplerSource, texture, false),
                             getAddress(info.SamplerSource, texture, true), isNearest(info.SamplerSource, texture), getTextureSize(texture)});
        }
    }
    return slots;
}

std::vector<MaterialTextureExtent> getTextureExtents(const UMaterialInterface& material, std::span<const UTexture* const> referenced) {
    std::vector<MaterialTextureExtent> extents;
    const auto add = [&](std::string name, int32 referencedIndex, const UTexture* texture) {
        const FTexturePlatformData* data = texture != nullptr ? texture->GetPlatformData() : nullptr;
        if (data != nullptr)
            extents.push_back({std::move(name), referencedIndex, data->SizeX, data->SizeY, data->GetNumSlices()});
    };
    TArray<FMaterialParameterInfo> parameters;
    material.GetAllParameterInfoOfType(EMaterialParameterType::Texture, parameters);
    for (const FMaterialParameterInfo& info : parameters) {
        UTexture* texture = nullptr;
        if (material.GetTextureParameterValue(info, texture))
            add(nameString(info.Name), INDEX_NONE, texture);
    }
    for (std::size_t index = 0; index < referenced.size(); ++index)
        add({}, static_cast<int32>(index), referenced[index]);
    return extents;
}

//! A shared shader map of UE 4.25 and later is in the library by its hash; before UE 4.25 the shader map keeps the output hash of each shader instead.
bool isInShaderMapLibrary(const FMaterialShaderMap& map) {
    return map.bShareCode && map.Code.ShaderHashes.IsEmpty();
}

bool hasShaderCode(const FMaterialShaderMap& map, const FShaderCodeLibrary& library) {
    if (isInShaderMapLibrary(map))
        return library.ContainsShaderMap(map.ResourceHash);
    return map.bShareCode || !map.Code.ShaderEntries.IsEmpty();
}

Result<std::vector<std::byte>> readShaderCode(const FMaterialShaderMap& map, const FShader& shader, const FShaderCodeLibrary& library) {
    if (isInShaderMapLibrary(map))
        return library.ReadShaderCode(map.ResourceHash, shader.ResourceIndex);
    if (map.bShareCode) {
        if (!map.Code.ShaderHashes.IsValidIndex(shader.ResourceIndex))
            return fail(ErrorCode::InvalidData, "ExportMaterialShaders.ShaderHashes", shader.ResourceIndex);
        return library.ReadShaderCode(map.Code.ShaderHashes[shader.ResourceIndex]);
    }
    if (!map.Code.ShaderEntries.IsValidIndex(shader.ResourceIndex))
        return fail(ErrorCode::InvalidData, "ExportMaterialShaders.ShaderEntries", shader.ResourceIndex);
    return library.ReadShaderCode(map.Code.ShaderEntries[shader.ResourceIndex]);
}

Result<FShaderCodeHash> getShaderHash(const FMaterialShaderMap& map, const FShader& shader, const FShaderCodeLibrary& library) {
    if (isInShaderMapLibrary(map))
        return library.GetShaderHash(map.ResourceHash, shader.ResourceIndex);
    if (!map.Code.ShaderHashes.IsValidIndex(shader.ResourceIndex))
        return fail(ErrorCode::InvalidData, "ExportMaterialShaders.ShaderHashes", shader.ResourceIndex);
    return map.Code.ShaderHashes[shader.ResourceIndex];
}

//! Before UE 4.11 the shader code names no uniform buffer, so each slot takes the shader variable name of the struct that its shader parameter binds.
Result<std::vector<std::string>> getLegacyUniformBufferNames(const FShader& shader, EGame game) {
    std::vector<std::string> names;
    if (game >= EGame::GAME_UE4_11)
        return names;
    const EngineShaderParameterLayouts* layouts = getEngineShaderParameterLayouts(game);
    if (layouts == nullptr)
        return fail(ErrorCode::UnsupportedVersion, "ExportMaterialShaders.EngineShaderParameterLayouts", static_cast<uint64>(game));
    const auto name = [&](const FShaderUniformBufferParameter parameter, std::string_view text) {
        if (names.size() <= parameter.BaseIndex)
            names.resize(parameter.BaseIndex + 1u);
        names[parameter.BaseIndex] = text;
    };
    if (shader.MaterialUniformBuffer.IsBound())
        name(shader.MaterialUniformBuffer, "Material");
    for (int32 index = 0; index < shader.ParameterCollectionUniformBuffers.Num(); ++index)
        if (shader.ParameterCollectionUniformBuffers[index].IsBound())
            name(shader.ParameterCollectionUniformBuffers[index], "MaterialCollection" + std::to_string(index));
    for (int32 index = 0; index < shader.UniformBufferParameters.Num(); ++index) {
        if (!shader.UniformBufferParameters[index].IsBound())
            continue;
        const std::string structName = nameString(shader.UniformBufferParameterStructNames[index]);
        const auto engine = std::ranges::find(layouts->buffers, structName, &EngineUniformBuffer::structName);
        if (engine == layouts->buffers.end())
            return fail(ErrorCode::InvalidData, "ExportMaterialShaders.UniformBufferParameterStructNames", static_cast<uint64>(index));
        name(shader.UniformBufferParameters[index], engine->name);
    }
    return names;
}

}

Result<std::optional<MaterialShaderMaps>> loadMaterialShaderMaps(const UMaterialInterface& material) {
    for (const UMaterialInterface* current = &material; current != nullptr;) {
        Result<TArray<FMaterialResource>> resources = current->LoadMaterialResources();
        if (!resources)
            return std::unexpected(resources.error());
        if (std::ranges::any_of(*resources, [](const FMaterialResource& resource) { return resource.ShaderMap.has_value(); }))
            return MaterialShaderMaps{current, std::move(*resources)};
        const auto* instance = Cast<UMaterialInstance>(current);
        current = instance != nullptr ? instance->Parent.Get() : nullptr;
    }
    return std::nullopt;
}

std::vector<MaterialShaderInfo> getMaterialShaders(const MaterialShaderMaps& maps, const FShaderCodeLibrary& library) {
    std::vector<MaterialShaderInfo> shaders;
    for (const FMaterialResource& resource : maps.resources) {
        if (!resource.ShaderMap || !hasShaderCode(*resource.ShaderMap, library))
            continue;
        const FMaterialShaderMap& map = *resource.ShaderMap;
        const ShaderTypeNames names(map.PointerTable);
        for (const FMeshMaterialShaderMap& mesh : map.Content.OrderedMeshShaderMaps)
            for (const FShader& shader : mesh.Shaders)
                shaders.push_back({&map, &mesh, &shader, std::string(names.find(map.PointerTable.GetShaderType(shader)))});
    }
    return shaders;
}

Result<MaterialShaderExport> exportMaterialShaders(const UMaterialInterface& material, const MaterialShaderMaps& maps,
                                                   std::span<const MaterialShaderInfo* const> shaders, const FShaderCodeLibrary& library) {
    if (shaders.empty() || maps.owner == nullptr)
        return fail(ErrorCode::InvalidData, "ExportMaterialShaders.Shaders");
    MaterialShaderExport exported;
    exported.shaderMap = shaders.front()->shaderMap;
    for (const MaterialShaderInfo* info : shaders) {
        if (info->shaderMap != exported.shaderMap)
            return fail(ErrorCode::InvalidData, "ExportMaterialShaders.ShaderMap");
        Result<std::vector<std::byte>> code = readShaderCode(*exported.shaderMap, *info->shader, library);
        if (!code)
            return std::unexpected(code.error());
        Result<runtime::FShaderCodeHash> hash = getShaderHash(*exported.shaderMap, *info->shader, library);
        if (!hash)
            return std::unexpected(hash.error());
        Result<std::vector<std::string>> names = getLegacyUniformBufferNames(*info->shader, GetRuntimeProvider().getGame());
        if (!names)
            return std::unexpected(names.error());
        exported.shaders.push_back({info->typeName, info->shader->GetFrequency(), std::move(*code), *hash, std::move(*names)});
    }

    const FUniformExpressionSet& expressions = exported.shaderMap->Content.MaterialCompilationOutput.UniformExpressionSet;
    const std::span<const uint8> defaults(expressions.DefaultValues.GetData(), static_cast<std::size_t>(expressions.DefaultValues.Num()));
    exported.numericParameters.reserve(static_cast<std::size_t>(expressions.UniformNumericParameters.Num()));
    for (const FMaterialNumericParameterInfo& numeric : expressions.UniformNumericParameters) {
        Result<MaterialNumericParameterValue> parameter = getNumericParameter(material, numeric, defaults);
        if (!parameter)
            return std::unexpected(parameter.error());
        exported.numericParameters.push_back(std::move(*parameter));
    }

    const std::vector<const UTexture*> referenced = getReferencedTextures(*maps.owner);
    exported.textureSlots = getTextureSlots(material, expressions, referenced);
    exported.textureExtents = getTextureExtents(material, referenced);
    exported.parameterCollections = getParameterCollections(*maps.owner, expressions.ParameterCollections);
    return exported;
}

}
