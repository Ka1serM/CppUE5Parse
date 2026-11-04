#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "CppUE5Parse/Core/Result.h"
#include "Engine/Source/Runtime/Engine/Engine/TextureDefines.h"
#include "Engine/Source/Runtime/Engine/MaterialShared.h"
#include "Engine/Source/Runtime/Engine/MaterialTypes.h"
#include "Engine/Source/Runtime/RenderCore/ShaderCodeArchive.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UMaterialParameterCollection;
class UTexture;

}

namespace ue5parse::conversion {

struct MaterialShaderMaps {
    const runtime::UMaterialInterface* owner = nullptr;
    runtime::TArray<runtime::FMaterialResource> resources;
};

struct MaterialShaderInfo {
    const runtime::FMaterialShaderMap* shaderMap = nullptr;
    const runtime::FMeshMaterialShaderMap* meshShaderMap = nullptr;
    const runtime::FShader* shader = nullptr;
    std::string typeName;
};

struct MaterialShaderCode {
    std::string typeName;
    runtime::EShaderFrequency frequency = runtime::SF_Pixel;
    std::vector<std::byte> code;
};

struct MaterialNumericParameterValue {
    std::string name;
    runtime::EMaterialParameterType type = runtime::EMaterialParameterType::Scalar;
    runtime::FLinearColor value;
};

struct MaterialTextureSlot {
    std::string parameterName;
    const runtime::UTexture* texture = nullptr;
    runtime::EMaterialTextureParameterType type = runtime::EMaterialTextureParameterType::Standard2D;
    int index = 0;
    runtime::TextureAddress addressX = runtime::TA_Wrap;
    runtime::TextureAddress addressY = runtime::TA_Wrap;
    bool nearest = false;
    std::array<int, 3> size{};
};

struct MaterialTextureExtent {
    std::string parameterName;
    int referencedTextureIndex = runtime::INDEX_NONE;
    int sizeX = 0;
    int sizeY = 0;
    int numSlices = 0;
};

struct MaterialShaderExport {
    const runtime::FMaterialShaderMap* shaderMap = nullptr;
    std::vector<MaterialShaderCode> shaders;
    std::vector<MaterialNumericParameterValue> numericParameters;
    std::vector<MaterialTextureSlot> textureSlots;
    std::vector<MaterialTextureExtent> textureExtents;
    std::vector<const runtime::UMaterialParameterCollection*> parameterCollections;
};

Result<std::optional<MaterialShaderMaps>> loadMaterialShaderMaps(const runtime::UMaterialInterface& material);

std::vector<MaterialShaderInfo> getMaterialShaders(const MaterialShaderMaps& maps, const runtime::FShaderCodeLibrary& library);

Result<MaterialShaderExport> exportMaterialShaders(const runtime::UMaterialInterface& material, const MaterialShaderMaps& maps,
                                                   std::span<const MaterialShaderInfo* const> shaders, const runtime::FShaderCodeLibrary& library);

}
