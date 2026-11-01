#pragma once

#include <optional>

#include "ue5parse/runtime/Core/Serialization/MemoryImage.h"

namespace ue5parse::runtime {

// UE's Shader.h, ShaderCore.h and RHIDefinitions.h: the cooked content of a shader map and its shader code (UE 5.3).
// The frozen types hold the fields of a cooked image; the editor-only fields are not in it.
//
// A frozen struct made only of intrinsics has its C++ layout (every alignment is at most MaxFieldAlignment, which is 8
// on a 64-bit target), so it is read as a whole. The other frozen structs give FrozenAlignment and an Unfreeze function.

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

// The frequency in the low 4 bits, the shader platform in the next 16 bits.
struct FShaderTarget {
    uint32 Packed = 0u;

    EShaderFrequency GetFrequency() const { return static_cast<EShaderFrequency>(Packed & 0xfu); }
    uint32 GetPlatform() const { return (Packed >> 4) & 0xffffu; }
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

// FShader: the base part of a shader object. The derived shader types add their own fields after it.
class FShader {
public:
    static constexpr uint32 FrozenAlignment = 8u;

    FShaderParameterBindings Bindings;
    FShaderParameterMapInfo ParameterMapInfo;
    TArray<FHashedName> UniformBufferParameterStructs;
    TArray<FShaderUniformBufferParameter> UniformBufferParameters;
    // Indices into the pointer table's shader types and vertex factory types; INDEX_NONE for none.
    int32 Type = INDEX_NONE;
    int32 VFType = INDEX_NONE;
    FShaderTarget Target;
    // The index of the shader's code in the shader map resource.
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

    // Parallel arrays: the type and permutation of each shader.
    TArray<FHashedName> ShaderTypes;
    TArray<int32> ShaderPermutations;
    TArray<FShader> Shaders;
    TArray<FShaderPipeline> ShaderPipelines;
    FName ShaderPlatformName;

    // The shader of a type and permutation, as FShaderMapContent::GetShader; null when the map has none. O(n): the
    // frozen hash table that UE uses is not kept.
    const FShader* GetShader(FHashedName TypeName, int32 PermutationId = 0) const;
};

// The shader map's pointer table: the shader types and vertex factory types that the shaders' indexed pointers refer
// to.
class FShaderMapPointerTable : public FPointerTableBase {
public:
    void LoadFromArchive(FArchive& Ar) override;

    TArray<FHashedName> ShaderTypes;
    TArray<FHashedName> VFTypes;

    FHashedName GetShaderType(const FShader& Shader) const {
        return ShaderTypes.IsValidIndex(Shader.Type) ? ShaderTypes[Shader.Type] : FHashedName();
    }
    FHashedName GetVertexFactoryType(const FShader& Shader) const {
        return VFTypes.IsValidIndex(Shader.VFType) ? VFTypes[Shader.VFType] : FHashedName();
    }
};

// The shader code that a cook without the shader library keeps inline in the shader map.
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

// FShaderMapBase::Serialize with the content type of TShaderMap: the frozen content, the shader platform, then where
// the code is. With bShareCode the code is in the shader library under ResourceHash; otherwise it is inline in Code.
template <class TContent>
class TShaderMap {
public:
    // A failure is set on the archive.
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
