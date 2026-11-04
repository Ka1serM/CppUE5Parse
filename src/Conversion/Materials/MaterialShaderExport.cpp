#include "Conversion/Materials/MaterialShaderExport.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture2D.h"
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

class ShaderTypeNames {
public:
    explicit ShaderTypeNames(const FShaderMapPointerTable& table) {
        for (const FPointerTableBase::FTypeDependency& dependency : table.GetTypeDependencies()) {
            std::string name = nameString(dependency.Name);
            names.emplace_back(FHashedName(std::string_view(name)).GetHash(), std::move(name));
        }
        std::ranges::sort(names);
    }

    std::string_view find(FHashedName name) const {
        const auto found = std::ranges::lower_bound(names, name.GetHash(), {}, &std::pair<uint64, std::string>::first);
        return found != names.end() && found->first == name.GetHash() ? std::string_view(found->second) : std::string_view();
    }

private:
    std::vector<std::pair<uint64, std::string>> names;
};

FMaterialParameterInfo toParameterInfo(const FHashedMaterialParameterInfo& info) {
    return {info.Name, info.Association, info.Index};
}

std::vector<const UTexture*> getReferencedTextures(const UMaterialInterface& owner) {
    std::vector<const UTexture*> result;
    TArray<UObject*> referenced;
    const void* data = owner.GetCachedExpressionData();
    if (data == nullptr || !GetPropertyValue_InContainer(UMaterialInterface::GetCachedExpressionDataStruct(), data, FName("ReferencedTextures"), referenced))
        return result;
    result.reserve(static_cast<std::size_t>(referenced.Num()));
    for (const UObject* object : referenced)
        result.push_back(Cast<UTexture>(object));
    return result;
}

template <class Visitor>
void forEachCachedElement(const UMaterialInterface& owner, const char* name, Visitor visit) {
    const UStruct* type = UMaterialInterface::GetCachedExpressionDataStruct();
    const void* container = owner.GetCachedExpressionData();
    const auto* array = FindFProperty<FArrayProperty>(type, FName(name));
    const auto* element = array != nullptr ? CastField<FStructProperty>(array->Inner) : nullptr;
    if (element == nullptr || container == nullptr)
        return;
    FScriptArrayHelper rows(array, array->ContainerPtrToValuePtr<void>(container));
    for (int32 index = 0; index < rows.Num(); ++index)
        visit(element->Struct, rows.GetRawPtr(index));
}

std::vector<const UMaterialParameterCollection*> getParameterCollections(const UMaterialInterface& owner, const TArray<FGuid>& ids) {
    std::vector<const UMaterialParameterCollection*> result(static_cast<std::size_t>(ids.Num()));
    forEachCachedElement(owner, "ParameterCollectionInfos", [&](const UStruct* type, const void* row) {
        FGuid id;
        UObject* object = nullptr;
        if (!GetPropertyValue_InContainer(type, row, FName("StateId"), id) || !GetPropertyValue_InContainer(type, row, FName("ParameterCollection"), object))
            return;
        const auto* collection = Cast<UMaterialParameterCollection>(object);
        if (collection == nullptr)
            return;
        for (int32 index = 0; index < ids.Num(); ++index)
            if (ids[index] == id)
                result[static_cast<std::size_t>(index)] = collection;
    });
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
        if (!resource.ShaderMap || !resource.ShaderMap->bShareCode || !library.ContainsShaderMap(resource.ShaderMap->ResourceHash))
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
        Result<std::vector<std::byte>> code = library.ReadShaderCode(exported.shaderMap->ResourceHash, info->shader->ResourceIndex);
        if (!code)
            return std::unexpected(code.error());
        exported.shaders.push_back({info->typeName, info->shader->GetFrequency(), std::move(*code)});
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
