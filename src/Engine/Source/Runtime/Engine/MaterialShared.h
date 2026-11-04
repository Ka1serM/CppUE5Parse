#pragma once

#include "Engine/Source/Runtime/Engine/MaterialTypes.h"
#include "Engine/Source/Runtime/RenderCore/Shader.h"

namespace ue5parse::runtime {

namespace ERHIFeatureLevel {

enum Type : int32 { ES2_REMOVED, ES3_1, SM4_REMOVED, SM5, SM6, Num };

}

enum class EMaterialQualityLevel : uint8 { Low, High, Medium, Epic, Num };

enum class EMaterialTextureParameterType : uint32 { Standard2D, Cube, Array2D, ArrayCube, Volume, Virtual, SparseVolume, Count };
inline constexpr uint32 NumMaterialTextureParameterTypes = static_cast<uint32>(EMaterialTextureParameterType::Count);

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
    uint8 Type;
};

struct FMaterialNumericParameterInfo {
    static constexpr uint32 FrozenAlignment = 4u;

    FHashedMaterialParameterInfo ParameterInfo;
    EMaterialParameterType ParameterType = EMaterialParameterType::Scalar;
    uint32 DefaultValueOffset = 0u;
};

struct FMaterialTextureParameterInfo {
    static constexpr uint32 FrozenAlignment = 4u;

    FHashedMaterialParameterInfo ParameterInfo;
    int32 TextureIndex = INDEX_NONE;
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

struct FPreshaderData {
    static constexpr uint32 FrozenAlignment = 8u;

    TArray<FName> Names;
    TArray<FPreshaderStructType> StructTypes;
    TArray<uint8> StructComponentTypes;
    TArray<uint8> Data;
};

struct FRHIUniformBufferResourceInitializer {
    uint16 MemberOffset;
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
    uint8 BindingFlags = 0u;
    bool bHasNonGraphOutputs = false;
    bool bNoEmulatedUniformBuffer = false;
};

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

class FMeshMaterialShaderMap : public FShaderMapContent {
public:
    FHashedName VertexFactoryTypeName;
};

class FMaterialShaderMapContent : public FShaderMapContent {
public:
    TArray<FMeshMaterialShaderMap> OrderedMeshShaderMaps;
    FMaterialCompilationOutput MaterialCompilationOutput;
    FSHAHash ShaderContentHash;
};

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
    void Serialize(FArchive& Ar);

    FMaterialShaderMapId ShaderMapId;
};

class FMaterialResource {
public:
    void SerializeInlineShaderMap(FArchive& Ar);

    std::optional<FMaterialShaderMap> ShaderMap;
};

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
