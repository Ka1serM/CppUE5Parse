#pragma once

#include "ue5parse/runtime/Engine/MaterialTypes.h"
#include "ue5parse/runtime/RenderCore/Shader.h"

namespace ue5parse::runtime {

// UE's MaterialShared.h, MaterialShader.h, Shader/Preshader.h, Rendering/StrataMaterialShared.h and
// RHIUniformBufferLayoutInitializer.h: the cooked shader maps of a material (UE 5.3). Cooked materials keep them inline
// after their properties; the code itself is in the shader libraries (FShaderCodeLibrary) or inline in each map.

namespace ERHIFeatureLevel {
enum Type : int32 { ES2_REMOVED, ES3_1, SM4_REMOVED, SM5, SM6, Num };
}

enum class EMaterialQualityLevel : uint8 { Low, High, Medium, Epic, Num };

enum class EMaterialTextureParameterType : uint32 { Standard2D, Cube, Array2D, ArrayCube, Volume, Virtual, SparseVolume, Count };
inline constexpr uint32 NumMaterialTextureParameterTypes = static_cast<uint32>(EMaterialTextureParameterType::Count);

// FHashedMaterialParameterInfo, frozen as FMemoryImageMaterialParameterInfo.
struct FHashedMaterialParameterInfo {
    static constexpr uint32 FrozenAlignment = 4u;

    FName Name;
    int32 Index = INDEX_NONE;
    EMaterialParameterAssociation Association = GlobalParameter;
};

struct FMaterialUniformPreshaderHeader {
    uint32 OpcodeOffset;
    uint32 OpcodeSize;
    uint32 FieldIndex;
    uint32 NumFields;
};

struct FMaterialUniformPreshaderField {
    uint32 BufferOffset;
    uint32 ComponentIndex;
    // UE::Shader::EValueType.
    uint8 Type;
};

struct FMaterialNumericParameterInfo {
    static constexpr uint32 FrozenAlignment = 4u;

    FHashedMaterialParameterInfo ParameterInfo;
    EMaterialParameterType ParameterType = EMaterialParameterType::Scalar;
    // The offset of the default value in FUniformExpressionSet::DefaultValues.
    uint32 DefaultValueOffset = 0u;
};

struct FMaterialTextureParameterInfo {
    static constexpr uint32 FrozenAlignment = 4u;

    FHashedMaterialParameterInfo ParameterInfo;
    // The index in the material's referenced textures.
    int32 TextureIndex = INDEX_NONE;
    // ESamplerSourceMode.
    uint8 SamplerSource = 0u;
    uint8 VirtualTextureLayerIndex = 0u;
};

struct FMaterialExternalTextureParameterInfo {
    static constexpr uint32 FrozenAlignment = 4u;

    FName ParameterName;
    FGuid ExternalTextureGuid;
    int32 SourceTextureIndex = INDEX_NONE;
};

struct FMaterialVirtualTextureStack {
    uint32 NumLayers;
    int32 LayerUniformExpressionIndices[8];
    int32 PreallocatedStackTextureIndex;
};

struct FPreshaderStructType {
    uint64 Hash;
    int32 ComponentTypeIndex;
    int32 NumComponents;
};

// UE::Shader::FPreshaderData: the opcodes of the preshaders that compute the uniform values.
struct FPreshaderData {
    static constexpr uint32 FrozenAlignment = 8u;

    TArray<FName> Names;
    TArray<FPreshaderStructType> StructTypes;
    // UE::Shader::EValueComponentType.
    TArray<uint8> StructComponentTypes;
    TArray<uint8> Data;
};

struct FRHIUniformBufferResourceInitializer {
    uint16 MemberOffset;
    // EUniformBufferBaseType.
    uint8 MemberType;
};

struct FRHIUniformBufferLayoutInitializer {
    static constexpr uint32 FrozenAlignment = 8u;

    FString Name;
    TArray<FRHIUniformBufferResourceInitializer> Resources;
    TArray<FRHIUniformBufferResourceInitializer> GraphResources;
    TArray<FRHIUniformBufferResourceInitializer> GraphTextures;
    TArray<FRHIUniformBufferResourceInitializer> GraphBuffers;
    TArray<FRHIUniformBufferResourceInitializer> GraphUniformBuffers;
    TArray<FRHIUniformBufferResourceInitializer> UniformBuffers;
    uint32 Hash = 0u;
    uint32 ConstantBufferSize = 0u;
    uint16 RenderTargetsOffset = 0xffffu;
    uint8 StaticSlot = 0xffu;
    // EUniformBufferBindingFlags.
    uint8 BindingFlags = 0u;
    bool bHasNonGraphOutputs = false;
    bool bNoEmulatedUniformBuffer = false;
};

// The uniform expressions of a material: its parameters, textures and the preshaders that fill its uniform buffer.
class FUniformExpressionSet {
public:
    static constexpr uint32 FrozenAlignment = 8u;

    TArray<FMaterialUniformPreshaderHeader> UniformPreshaders;
    TArray<FMaterialUniformPreshaderField> UniformPreshaderFields;
    TArray<FMaterialNumericParameterInfo> UniformNumericParameters;
    TArray<FMaterialTextureParameterInfo> UniformTextureParameters[NumMaterialTextureParameterTypes];
    TArray<FMaterialExternalTextureParameterInfo> UniformExternalTextureParameters;
    uint32 UniformPreshaderBufferSize = 0u;
    FPreshaderData UniformPreshaderData;
    TArray<uint8> DefaultValues;
    TArray<FMaterialVirtualTextureStack> VTStacks;
    TArray<FGuid> ParameterCollections;
    FRHIUniformBufferLayoutInitializer UniformBufferLayoutInitializer;
};

struct FStrataMaterialCompilationOutput {
    static constexpr uint32 FrozenAlignment = 1u;

    uint8 StrataMaterialType = 0u;
    uint8 StrataBSDFCount = 0u;
    uint8 StrataUintPerPixel = 0u;
    bool bUsesComplexSpecialRenderPath = false;
};

class FMaterialCompilationOutput {
public:
    static constexpr uint32 FrozenAlignment = 8u;

    FUniformExpressionSet UniformExpressionSet;
    uint32 UsedSceneTextures = 0u;
    uint8 UsedPathTracingBufferTextures = 0u;
    FStrataMaterialCompilationOutput StrataMaterialCompilationOutput;
    uint8 UsedDBufferTextures = 0u;
    uint8 RuntimeVirtualTextureOutputAttributeMask = 0u;
    bool bNeedsSceneTextures = false;
    bool bUsesDBufferTextureLookup = false;
    bool bUsesEyeAdaptation = false;
    bool bModifiesMeshPosition = false;
    bool bUsesWorldPositionOffset = false;
    bool bUsesGlobalDistanceField = false;
    bool bUsesPixelDepthOffset = false;
    bool bUsesDistanceCullFade = false;
    bool bUsesPerInstanceCustomData = false;
    bool bUsesPerInstanceRandom = false;
    bool bUsesVertexInterpolator = false;
    bool bHasRuntimeVirtualTextureOutputNode = false;
    bool bUsesAnisotropy = false;
    bool bUsesDisplacement = false;
};

// The shaders of a material for one vertex factory.
class FMeshMaterialShaderMap : public FShaderMapContent {
public:
    FHashedName VertexFactoryTypeName;
};

class FMaterialShaderMapContent : public FShaderMapContent {
public:
    // The mesh shader maps, one for each vertex factory.
    TArray<FMeshMaterialShaderMap> OrderedMeshShaderMaps;
    FMaterialCompilationOutput MaterialCompilationOutput;
    FSHAHash ShaderContentHash;
};

// The id of a cooked shader map: the quality and feature level it is for.
class FMaterialShaderMapId {
public:
    void Serialize(FArchive& Ar);

    EMaterialQualityLevel QualityLevel = EMaterialQualityLevel::Num;
    ERHIFeatureLevel::Type FeatureLevel = ERHIFeatureLevel::Num;
    FSHAHash CookedShaderMapIdHash;
    FPlatformTypeLayoutParameters LayoutParams;
};

class FMaterialShaderMap : public TShaderMap<FMaterialShaderMapContent> {
public:
    // A failure is set on the archive.
    void Serialize(FArchive& Ar);

    FMaterialShaderMapId ShaderMapId;
};

// The material resource of one quality and feature level, with its shader map when the cook saved a valid one.
class FMaterialResource {
public:
    // FMaterial::SerializeInlineShaderMap. A failure is set on the archive.
    void SerializeInlineShaderMap(FArchive& Ar);

    std::optional<FMaterialShaderMap> ShaderMap;
};

// The inline shader maps of a cooked material (Material.cpp's SerializeInlineShaderMaps): the resource count, then the
// resources through FMaterialResourceProxyReader. Ar is at the resource count. A failure is set on Ar.
void SerializeInlineShaderMaps(FArchive& Ar, TArray<FMaterialResource>& OutLoadedResources);

void Unfreeze(FMemoryImageReader& Reader, FHashedMaterialParameterInfo& Value);
void Unfreeze(FMemoryImageReader& Reader, FMaterialNumericParameterInfo& Value);
void Unfreeze(FMemoryImageReader& Reader, FMaterialTextureParameterInfo& Value);
void Unfreeze(FMemoryImageReader& Reader, FMaterialExternalTextureParameterInfo& Value);
void Unfreeze(FMemoryImageReader& Reader, FPreshaderData& Value);
void Unfreeze(FMemoryImageReader& Reader, FRHIUniformBufferLayoutInitializer& Value);
void Unfreeze(FMemoryImageReader& Reader, FUniformExpressionSet& Value);
void Unfreeze(FMemoryImageReader& Reader, FStrataMaterialCompilationOutput& Value);
void Unfreeze(FMemoryImageReader& Reader, FMaterialCompilationOutput& Value);
void Unfreeze(FMemoryImageReader& Reader, FMeshMaterialShaderMap& Value);
void Unfreeze(FMemoryImageReader& Reader, FMaterialShaderMapContent& Value);

}
