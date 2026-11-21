#include "Engine/Source/Runtime/RenderCore/Shader.h"

#include <algorithm>

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

static_assert(sizeof(FShaderParameterBindings::FParameter) == 8 && sizeof(FShaderParameterBindings::FResourceParameter) == 4
              && sizeof(FShaderParameterBindings::FBindlessResourceParameter) == 6 && sizeof(FShaderParameterBindings::FParameterStructReference) == 4);
static_assert(sizeof(FShaderResourceParameterInfo) == 4 && sizeof(FShaderLooseParameterInfo) == 4 && sizeof(FShaderParameterInfo) == 4);

void Unfreeze(FMemoryImageReader& Reader, FShaderParameterBindings& Value) {
    Reader.ReadArray(Value.Parameters);
    if (Reader.GetGame() < EGame::GAME_UE4_26) {
        //! UE 4.25 keeps textures, SRVs, UAVs, samplers and their graph variants in seven arrays of another element type.
        for (int32 Index = 0; Index < 7; Index++)
            Reader.SkipArray();
    } else {
        Reader.ReadArray(Value.ResourceParameters);
    }
    if (Reader.GetGame() >= EGame::GAME_UE5_1)
        Reader.ReadArray(Value.BindlessResourceParameters);
    if (Reader.GetGame() >= EGame::GAME_UE4_26)
        Reader.ReadArray(Value.GraphUniformBuffers);
    Reader.ReadArray(Value.ParameterReferences);
    Value.StructureLayoutHash = Reader.Read<uint32>();
    Value.RootParameterBufferIndex = Reader.Read<uint16>();
}

void Unfreeze(FMemoryImageReader& Reader, FShaderLooseParameterBufferInfo& Value) {
    Value.BaseIndex = Reader.Read<uint16>();
    Value.Size = Reader.Read<uint16>();
    Reader.ReadArray(Value.Parameters);
}

void Unfreeze(FMemoryImageReader& Reader, FShaderParameterMapInfo& Value) {
    if (Reader.GetGame() >= EGame::GAME_UE5_1) {
        Reader.ReadArray(Value.UniformBuffers);
        Reader.ReadArray(Value.TextureSamplers);
        Reader.ReadArray(Value.SRVs);
    } else {
        Reader.ReadArray(Value.LegacyUniformBuffers);
        Reader.ReadArray(Value.LegacyTextureSamplers);
        Reader.ReadArray(Value.LegacySRVs);
    }
    Reader.ReadArray(Value.LooseParameterBuffers);
    if (Reader.GetGame() >= EGame::GAME_UE4_26)
        Value.Hash = Reader.Read<uint64>();
}

void Unfreeze(FMemoryImageReader& Reader, FShader& Value) {
    Reader.ReadStruct(Value.Bindings);
    Reader.ReadStruct(Value.ParameterMapInfo);
    Reader.ReadArray(Value.UniformBufferParameterStructs);
    Reader.ReadArray(Value.UniformBufferParameters);
    Value.Type = Reader.ReadIndexedPointer();
    Value.VFType = Reader.ReadIndexedPointer();
    Value.Target = Reader.Read<FShaderTarget>();
    Value.ResourceIndex = Reader.Read<int32>();
    Value.NumInstructions = Reader.Read<uint32>();
    if (Reader.GetGame() >= EGame::GAME_UE5_0 && Reader.GetGame() < EGame::GAME_UE5_9)
        Value.SortKey = Reader.Read<uint32>();
}

namespace {

void ReadShaderPointer(FMemoryImageReader& Reader, std::optional<FShader>& Out) {
    std::optional<FMemoryImageReader> Target = Reader.ReadPointer();
    if (!Target)
        return;
    Target->ReadStruct(Out.emplace());
}

}

void Unfreeze(FMemoryImageReader& Reader, FShaderPipeline& Value) {
    Value.TypeName = Reader.Read<FHashedName>();
    for (std::optional<FShader>& Shader : Value.Shaders)
        ReadShaderPointer(Reader, Shader);
    for (int32& PermutationId : Value.PermutationIds)
        PermutationId = Reader.Read<int32>();
}

void Unfreeze(FMemoryImageReader& Reader, FShaderMapContent& Value) {
    Reader.ReadPointer();
    Reader.ReadPointer();
    Reader.Read<uint32>();
    Reader.Read<uint32>();
    Reader.ReadArray(Value.ShaderTypes);
    Reader.ReadArray(Value.ShaderPermutations);
    Reader.ReadArray(Value.Shaders, FShader::FrozenAlignment, [](FMemoryImageReader& Element, FShader& Shader) {
        std::optional<FMemoryImageReader> Target = Element.ReadPointer();
        if (!Target) {
            Element.SetError("FShaderMapContent.Shaders");
            return;
        }
        Target->ReadStruct(Shader);
    });
    Reader.ReadArray(Value.ShaderPipelines, FShaderPipeline::FrozenAlignment, [](FMemoryImageReader& Element, FShaderPipeline& Pipeline) {
        std::optional<FMemoryImageReader> Target = Element.ReadPointer();
        if (!Target) {
            Element.SetError("FShaderMapContent.ShaderPipelines");
            return;
        }
        Target->ReadStruct(Pipeline);
    });
    if (Value.ShaderTypes.Num() != Value.Shaders.Num() || Value.ShaderPermutations.Num() != Value.Shaders.Num())
        Reader.SetError("FShaderMapContent.ShaderTypes");
    if (Reader.GetGame() >= EGame::GAME_UE5_2)
        Value.ShaderPlatformName = Reader.ReadMemoryImageName();
    else
        Value.ShaderPlatform = Reader.Read<uint8>();
    if (Reader.GetGame() == EGame::GAME_Valorant)
        Reader.Read<uint64>();
}

void FShaderMapPointerTable::LoadFromArchive(FArchive& Ar) {
    FPointerTableBase::LoadFromArchive(Ar);
    int32 NumTypes = Ar.ReadCount(8);
    int32 NumVFTypes = Ar.ReadCount(8);
    SerializeRaw(Ar, ShaderTypes, NumTypes);
    SerializeRaw(Ar, VFTypes, NumVFTypes);
}

FArchive& operator<<(FArchive& Ar, FShaderMapResourceCode::FShaderEntry& Entry) {
    uint8 Frequency = 0;
    Ar << Entry.Code << Entry.UncompressedSize << Frequency;
    Entry.Frequency = static_cast<EShaderFrequency>(Frequency);
    return Ar;
}

void SkipHashedTypeDependencies(FArchive& Ar) {
    constexpr int32 HashedTypeDependencySize = static_cast<int32>(sizeof(uint64) + sizeof(uint32) + sizeof(FSHAHash));
    int32 NumDependencies = Ar.ReadCount(HashedTypeDependencySize);
    Ar.SkipBytes(static_cast<int64>(NumDependencies) * HashedTypeDependencySize);
}

void FShaderMapResourceCode::Serialize(FArchive& Ar) {
    Ar << ResourceHash;
    Ar << ShaderHashes;
    Ar << ShaderEntries;
    if (!Ar.IsError() && ShaderEntries.Num() != ShaderHashes.Num())
        Ar.SetError(ErrorCode::InvalidData, "FShaderMapResourceCode.ShaderEntries");
}


namespace {

struct FLegacyShader {
    FShader Shader;
    FName TypeName;
    FName VFTypeName;
    int32 PermutationId = 0;
    FSHAHash OutputHash;
    FSHAHash ResourceOutputHash;
    FShaderTarget ResourceTarget;
    FShaderMapResourceCode::FShaderEntry Entry;
    bool bCodeShared = false;

    bool IsResourceOfShader() const { return ResourceOutputHash == OutputHash && ResourceTarget.Packed == Shader.Target.Packed; }
};

class FShaderLayoutReader final : public FMemoryReaderView {
public:
    FShaderLayoutReader(std::span<const std::byte> InBytes, const FArchive& Source) : FMemoryReaderView(InBytes) { CopyStateFrom(Source); }

    using FArchive::operator<<;
    FArchive& operator<<(FName& Value) override {
        SkipBytes(static_cast<int64>(sizeof(uint32) * 2));
        Value = FName();
        return *this;
    }
};

constexpr int64 SerializedNameSize = static_cast<int64>(sizeof(uint32) * 2);

void SkipArray(FArchive& Ar, int32 ElementSize) {
    int32 Count = Ar.ReadCount(ElementSize);
    Ar.SkipBytes(static_cast<int64>(Count) * ElementSize);
}

bool HasPermutationIds(const FArchive& Ar) {
    return Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::ShaderPermutationId, EGame::GAME_UE4_20);
}

bool HasSharedShaderCode(const FArchive& Ar) {
    return Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::ShaderResourceCodeSharing, EGame::GAME_UE4_16);
}

FShaderTarget ReadLegacyTarget(FArchive& Ar) {
    uint32 Frequency = Ar.Read<uint32>();
    uint32 Platform = Ar.Read<uint32>();
    return FShaderTarget{(Frequency & 0xfu) | (Platform << 4)};
}

void SkipLegacyParameterMapInfo(FArchive& Ar) {
    constexpr int32 ParameterInfoSize = static_cast<int32>(sizeof(FShaderParameterInfo));
    for (int32 Index = 0; Index < 3; Index++)
        SkipArray(Ar, ParameterInfoSize);
    int32 NumLooseBuffers = Ar.ReadCount(8);
    for (int32 Index = 0; Index < NumLooseBuffers && !Ar.IsError(); Index++) {
        Ar.SkipBytes(static_cast<int64>(sizeof(uint16) * 2));
        SkipArray(Ar, ParameterInfoSize);
    }
}

void SkipLegacyBindings(FArchive& Ar) {
    SkipArray(Ar, static_cast<int32>(sizeof(FShaderParameterBindings::FParameter)));
    const int32 NumResourceArrays = Ar.GetGame() >= EGame::GAME_UE4_23 ? 7 : 6;
    for (int32 Index = 0; Index < NumResourceArrays; Index++)
        SkipArray(Ar, static_cast<int32>(sizeof(uint16) * 2));
    SkipArray(Ar, static_cast<int32>(sizeof(FShaderParameterBindings::FParameterStructReference)));
    if (Ar.GetGame() >= EGame::GAME_UE4_24)
        Ar.Read<uint32>();
    Ar.Read<uint16>();
}

void SerializeLegacyShaderResource(FArchive& Ar, FLegacyShader& Value) {
    if (Ar.GetGame() >= EGame::GAME_UE4_8) {
        FName SpecificType;
        Ar << SpecificType;
    }
    if (HasPermutationIds(Ar))
        Ar.Read<int32>();
    Value.ResourceTarget = ReadLegacyTarget(Ar);
    const bool bSharedCodeVersion = HasSharedShaderCode(Ar);
    if (!bSharedCodeVersion)
        Ar << Value.Entry.Code;
    Ar << Value.ResourceOutputHash;
    Ar << Value.Shader.NumInstructions;
    if (Ar.GetGame() < EGame::GAME_UE4_22)
        Ar.Read<uint32>();
    else
        SkipLegacyParameterMapInfo(Ar);
    Value.Entry.UncompressedSize = Value.Entry.Code.Num();
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::COMPRESSED_SHADER_RESOURCES)
        Ar << Value.Entry.UncompressedSize;
    if (!bSharedCodeVersion)
        return;
    Ar << Value.bCodeShared;
    if (!Value.bCodeShared)
        Ar << Value.Entry.Code;
}

void SerializeLegacyShaderBase(FArchive& Ar, FLegacyShader& Value) {
    FSHAHash Hash;
    Ar << Value.OutputHash << Hash;
    if (Ar.GetGame() >= EGame::GAME_UE4_11) {
        FName PipelineName;
        Ar << PipelineName;
    }
    Ar << Value.VFTypeName << Hash << Value.TypeName;
    if (HasPermutationIds(Ar))
        Ar << Value.PermutationId;
    Ar << Hash;
    Value.Shader.Target = ReadLegacyTarget(Ar);
    Value.Entry.Frequency = Value.Shader.GetFrequency();
    const bool bStructNames = Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID,
                                                  FFortniteMainBranchObjectVersion::MaterialInstanceSerializeOptimization_ShaderFName, EGame::GAME_UE4_21);
    int32 NumUniformParameters = Ar.ReadCount(10);
    for (int32 Index = 0; Index < NumUniformParameters && !Ar.IsError(); Index++) {
        FName StructName;
        if (bStructNames) {
            Ar << StructName;
        } else {
            FString StructText;
            Ar << StructText;
            StructName = MakeName(StructText);
        }
        bool bIsBound = false;
        FShaderUniformBufferParameter Parameter{Ar.Read<uint16>()};
        Ar << bIsBound;
        Value.Shader.UniformBufferParameterStructs.Add(FHashedName(StructName));
        Value.Shader.UniformBufferParameters.Add(Parameter);
    }
    SerializeLegacyShaderResource(Ar, Value);
    if (Ar.GetGame() >= EGame::GAME_UE4_22)
        SkipLegacyBindings(Ar);
}

int64 GetLegacyTypeNameOffset(const FArchive& Ar) {
    const int64 NumNamesBefore = Ar.GetGame() >= EGame::GAME_UE4_11 ? 2 : 1;
    return static_cast<int64>(sizeof(FSHAHash)) * 3 + NumNamesBefore * SerializedNameSize;
}

bool IsLegacyShaderBaseAt(std::span<const std::byte> Bytes, int64 BaseOffset, std::span<const std::byte> TypeNameBytes, const FArchive& Ar) {
    const int64 TypeNameOffset = BaseOffset + GetLegacyTypeNameOffset(Ar);
    if (!std::ranges::equal(Bytes.subspan(static_cast<std::size_t>(TypeNameOffset), TypeNameBytes.size()), TypeNameBytes))
        return false;
    FShaderLayoutReader Layout(Bytes.subspan(static_cast<std::size_t>(BaseOffset)), Ar);
    FLegacyShader Probe;
    SerializeLegacyShaderBase(Layout, Probe);
    return !Layout.IsError() && Layout.GetRemaining() == 0 && Probe.IsResourceOfShader();
}

//! A shader class writes its own parameters before FShader::SerializeBase; their layout is not in the file, so the base is found by its type name and its resource.
void SerializeLegacyShader(FArchive& Ar, std::span<const std::byte> TypeNameBytes, FLegacyShader& Value) {
    const int64 EndOffset = Ar.GetGame() >= EGame::GAME_UE4_20 ? Ar.Read<int64>() : Ar.Read<int32>();
    const int64 Start = Ar.Tell();
    const int64 End = EndOffset - (Ar.GetCookedPosition() - Start);
    if (Ar.IsError() || End < Start || End > Ar.TotalSize()) {
        Ar.SetError(ErrorCode::InvalidData, "FShader.EndOffset");
        return;
    }
    std::span<const std::byte> Bytes = Ar.ReadView(End - Start);
    const int64 LastBaseOffset = static_cast<int64>(Bytes.size()) - GetLegacyTypeNameOffset(Ar) - SerializedNameSize;
    for (int64 BaseOffset = 0; BaseOffset <= LastBaseOffset && !Ar.IsError(); BaseOffset++) {
        if (!IsLegacyShaderBaseAt(Bytes, BaseOffset, TypeNameBytes, Ar))
            continue;
        Ar.Seek(Start + BaseOffset);
        SerializeLegacyShaderBase(Ar, Value);
        return;
    }
    Ar.SetError(ErrorCode::InvalidData, "FShader.SerializeBase");
}

std::span<const std::byte> ReadTypeNameBytes(FArchive& Ar, FName& TypeName) {
    const int64 TypeNameOffset = Ar.Tell();
    Ar << TypeName;
    if (Ar.IsError())
        return {};
    Ar.Seek(TypeNameOffset);
    return Ar.ReadView(SerializedNameSize);
}

int32 FindOrAddType(TArray<FHashedName>& Types, FName Name) {
    const FHashedName HashedName(Name);
    for (int32 Index = 0; Index < Types.Num(); Index++)
        if (Types[Index] == HashedName)
            return Index;
    return Types.Add(HashedName);
}

class FLegacyShaderMapContentReader {
public:
    FLegacyShaderMapContentReader(FArchive& InAr, FShaderMapPointerTable& InPointerTable, FShaderMapResourceCode& InCode, bool& bInShareCode)
        : Ar(InAr), PointerTable(InPointerTable), Code(InCode), bShareCode(bInShareCode) {}

    void Read(FShaderMapContent& Content) {
        int32 NumShaders = Ar.ReadCount(12);
        for (int32 Index = 0; Index < NumShaders && !Ar.IsError(); Index++) {
            FLegacyShader Shader;
            if (!ReadShader(Shader))
                return;
            Content.ShaderTypes.Add(FHashedName(Shader.TypeName));
            Content.ShaderPermutations.Add(Shader.PermutationId);
            Content.Shaders.Add(std::move(Shader.Shader));
        }
        if (Ar.GetGame() < EGame::GAME_UE4_11)
            return;
        int32 NumPipelines = Ar.ReadCount(12);
        for (int32 Index = 0; Index < NumPipelines && !Ar.IsError(); Index++)
            ReadPipeline(Content.ShaderPipelines[Content.ShaderPipelines.AddDefaulted()]);
    }

private:
    void ReadPipeline(FShaderPipeline& Pipeline) {
        FName TypeName;
        Ar << TypeName;
        Pipeline.TypeName = FHashedName(TypeName);
        int32 NumStages = Ar.ReadCount(12);
        for (int32 Index = 0; Index < NumStages && !Ar.IsError(); Index++) {
            FLegacyShader Stage;
            if (!ReadShader(Stage))
                return;
            const EShaderFrequency Frequency = Stage.Shader.GetFrequency();
            if (Frequency >= SF_NumGraphicsFrequencies || Pipeline.Shaders[Frequency]) {
                Ar.SetError(ErrorCode::InvalidData, "FShaderPipeline.Stages");
                return;
            }
            Pipeline.PermutationIds[Frequency] = Stage.PermutationId;
            Pipeline.Shaders[Frequency] = std::move(Stage.Shader);
        }
    }

    bool ReadShader(FLegacyShader& Shader) {
        FName TypeName;
        std::span<const std::byte> TypeNameBytes = ReadTypeNameBytes(Ar, TypeName);
        SerializeLegacyShader(Ar, TypeNameBytes, Shader);
        if (Ar.IsError())
            return false;
        Shader.Shader.Type = FindOrAddType(PointerTable.ShaderTypes, Shader.TypeName);
        if (!Shader.VFTypeName.isNone())
            Shader.Shader.VFType = FindOrAddType(PointerTable.VFTypes, Shader.VFTypeName);
        AddResource(Shader);
        return !Ar.IsError();
    }

    void AddResource(FLegacyShader& Shader) {
        if (Code.ShaderEntries.IsEmpty())
            bShareCode = Shader.bCodeShared;
        if (Shader.bCodeShared != bShareCode) {
            Ar.SetError(ErrorCode::InvalidData, "FShaderResource.bCodeShared");
            return;
        }
        Shader.Shader.ResourceIndex = Code.ShaderEntries.Add(std::move(Shader.Entry));
        Code.ShaderHashes.Add(FShaderCodeHash(Shader.OutputHash));
    }

    FArchive& Ar;
    FShaderMapPointerTable& PointerTable;
    FShaderMapResourceCode& Code;
    bool& bShareCode;
};

}

void SerializeLegacyShaderMapContent(FArchive& Ar, FShaderMapContent& Content, FShaderMapPointerTable& PointerTable, FShaderMapResourceCode& Code,
                                     bool& bShareCode) {
    FLegacyShaderMapContentReader(Ar, PointerTable, Code, bShareCode).Read(Content);
}

}
