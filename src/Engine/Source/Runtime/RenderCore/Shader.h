#pragma once

#include <optional>

#include "Engine/Source/Runtime/Core/Serialization/MemoryImage.h"

namespace ue5parse::runtime {

//! A frozen struct of intrinsics only has its C++ layout (alignments at most MaxFieldAlignment, 8 on 64-bit): read as a whole.

enum EShaderFrequency : uint8 {
    SF_Vertex = 0,
    SF_Mesh = 1,
    SF_Amplification = 2,
    SF_Pixel = 3,
    SF_Geometry = 4,
    SF_Compute = 5,
    SF_RayGen = 6,
    SF_RayMiss = 7,
    SF_RayHitGroup = 8,
    SF_RayCallable = 9,
    SF_NumFrequencies = 10,
    SF_NumGraphicsFrequencies = 5,
};

struct FShaderTarget {
    uint32 Packed = 0u;

    EShaderFrequency GetFrequency() const { return static_cast<EShaderFrequency>(Packed & 0xfu); }
};

class FShaderParameterBindings {
public:
    struct FParameter {
        uint16 BufferIndex;
        uint16 BaseIndex;
        uint16 ByteOffset;
        uint16 ByteSize;
    };
    struct FResourceParameter {
        uint16 ByteOffset;
        uint8 BaseIndex;
        uint8 BaseType;
    };
    struct FBindlessResourceParameter {
        uint16 ByteOffset;
        uint16 GlobalConstantOffset;
        uint8 BaseType;
    };
    struct FParameterStructReference {
        uint16 BufferIndex;
        uint16 ByteOffset;
    };

    static constexpr uint32 FrozenAlignment = 8u;

    TArray<FParameter> Parameters;
    TArray<FResourceParameter> ResourceParameters;
    TArray<FBindlessResourceParameter> BindlessResourceParameters;
    TArray<FParameterStructReference> GraphUniformBuffers;
    TArray<FParameterStructReference> ParameterReferences;
    uint32 StructureLayoutHash = 0u;
    uint16 RootParameterBufferIndex = 0xffffu;
};

struct FShaderUniformBufferParameterInfo {
    uint16 BaseIndex;
};

struct FShaderResourceParameterInfo {
    uint16 BaseIndex;
    uint8 BufferIndex;
    uint8 Type;
};

struct FShaderLooseParameterInfo {
    uint16 BaseIndex;
    uint16 Size;
};

struct FShaderLooseParameterBufferInfo {
    static constexpr uint32 FrozenAlignment = 8u;

    uint16 BaseIndex = 0u;
    uint16 Size = 0u;
    TArray<FShaderLooseParameterInfo> Parameters;
};

struct FShaderParameterMapInfo {
    static constexpr uint32 FrozenAlignment = 8u;

    TArray<FShaderUniformBufferParameterInfo> UniformBuffers;
    TArray<FShaderResourceParameterInfo> TextureSamplers;
    TArray<FShaderResourceParameterInfo> SRVs;
    TArray<FShaderLooseParameterBufferInfo> LooseParameterBuffers;
    uint64 Hash = 0u;
};

struct FShaderUniformBufferParameter {
    uint16 BaseIndex;
};

class FShader {
public:
    static constexpr uint32 FrozenAlignment = 8u;

    FShaderParameterBindings Bindings;
    FShaderParameterMapInfo ParameterMapInfo;
    TArray<FHashedName> UniformBufferParameterStructs;
    TArray<FShaderUniformBufferParameter> UniformBufferParameters;
    int32 Type = INDEX_NONE;
    int32 VFType = INDEX_NONE;
    FShaderTarget Target;
    int32 ResourceIndex = INDEX_NONE;
    uint32 NumInstructions = 0u;
    uint32 SortKey = 0u;

    EShaderFrequency GetFrequency() const { return Target.GetFrequency(); }
};

class FShaderPipeline {
public:
    static constexpr uint32 FrozenAlignment = 8u;

    FHashedName TypeName;
    std::optional<FShader> Shaders[SF_NumGraphicsFrequencies];
    int32 PermutationIds[SF_NumGraphicsFrequencies] = {};
};

class FShaderMapContent {
public:
    static constexpr uint32 FrozenAlignment = 8u;

    TArray<FHashedName> ShaderTypes;
    TArray<int32> ShaderPermutations;
    TArray<FShader> Shaders;
    TArray<FShaderPipeline> ShaderPipelines;
    FName ShaderPlatformName;
};

class FShaderMapPointerTable : public FPointerTableBase {
public:
    void LoadFromArchive(FArchive& Ar) override;

    TArray<FHashedName> ShaderTypes;
    TArray<FHashedName> VFTypes;

    FHashedName GetShaderType(const FShader& Shader) const { return ShaderTypes.IsValidIndex(Shader.Type) ? ShaderTypes[Shader.Type] : FHashedName(); }
};

class FShaderMapResourceCode {
public:
    struct FShaderEntry {
        TArray<uint8> Code;
        int32 UncompressedSize = 0;
        EShaderFrequency Frequency = SF_Vertex;
    };

    void Serialize(FArchive& Ar);

    FSHAHash ResourceHash;
    TArray<FSHAHash> ShaderHashes;
    TArray<FShaderEntry> ShaderEntries;
};

FArchive& operator<<(FArchive& Ar, FShaderMapResourceCode::FShaderEntry& Entry);

template <class TContent>
class TShaderMap {
public:
    void Serialize(FArchive& Ar) {
        FMemoryImageObject Image = FMemoryImageResult::LoadFromArchive(Ar, PointerTable);
        LayoutParameters = Image.GetLayoutParameters();
        if (Ar.IsError())
            return;
        const char* FailedCheck = nullptr;
        FMemoryImageReader Reader(Image, FailedCheck);
        Reader.ReadStruct(Content);
        if (FailedCheck != nullptr) {
            Ar.SetError(ErrorCode::InvalidData, FailedCheck);
            return;
        }
        Ar << bShareCode << ShaderPlatformName;
        if (bShareCode)
            Ar << ResourceHash;
        else
            Code.Serialize(Ar);
    }

    TContent Content;
    FShaderMapPointerTable PointerTable;
    FPlatformTypeLayoutParameters LayoutParameters;
    bool bShareCode = false;
    FName ShaderPlatformName;
    FSHAHash ResourceHash;
    FShaderMapResourceCode Code;
};

void Unfreeze(FMemoryImageReader& Reader, FShaderParameterBindings& Value);
void Unfreeze(FMemoryImageReader& Reader, FShaderLooseParameterBufferInfo& Value);
void Unfreeze(FMemoryImageReader& Reader, FShaderParameterMapInfo& Value);
void Unfreeze(FMemoryImageReader& Reader, FShader& Value);
void Unfreeze(FMemoryImageReader& Reader, FShaderPipeline& Value);
void Unfreeze(FMemoryImageReader& Reader, FShaderMapContent& Value);

}
