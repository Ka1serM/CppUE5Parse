#include "ue5parse/runtime/RenderCore/Shader.h"

namespace ue5parse::runtime {

// UE's Shader.cpp, ShaderMap.cpp and ShaderResource.cpp: the frozen fields of the shader map content and the shader map
// archive data (UE 5.3).

static_assert(sizeof(FShaderParameterBindings::FParameter) == 8 && sizeof(FShaderParameterBindings::FResourceParameter) == 4 &&
              sizeof(FShaderParameterBindings::FBindlessResourceParameter) == 6 &&
              sizeof(FShaderParameterBindings::FParameterStructReference) == 4);
static_assert(sizeof(FShaderResourceParameterInfo) == 4 && sizeof(FShaderLooseParameterInfo) == 4);

void Unfreeze(FMemoryImageReader& Reader, FShaderParameterBindings& Value) {
    Reader.ReadArray(Value.Parameters);
    Reader.ReadArray(Value.ResourceParameters);
    Reader.ReadArray(Value.BindlessResourceParameters);
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
    Reader.ReadArray(Value.UniformBuffers);
    Reader.ReadArray(Value.TextureSamplers);
    Reader.ReadArray(Value.SRVs);
    Reader.ReadArray(Value.LooseParameterBuffers);
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
    Value.SortKey = Reader.Read<uint32>();
}

namespace {

// TMemoryImagePtr<FShader>: the shader object it points to. The derived shader types put their fields after the base.
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
    // FMemoryImageHashTable ShaderHash: the hash of the type and permutation of each shader, as two frozen pointers and
    // two sizes. GetShader searches the arrays instead.
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
    Value.ShaderPlatformName = Reader.ReadMemoryImageName();
    // VALORANT: 8 more bytes after the shader platform name.
    if (Reader.GetGame() == EGame::GAME_Valorant)
        Reader.Read<uint64>();
}

const FShader* FShaderMapContent::GetShader(FHashedName TypeName, int32 PermutationId) const {
    for (int32 Index = 0; Index < Shaders.Num(); Index++)
        if (ShaderTypes[Index] == TypeName && ShaderPermutations[Index] == PermutationId)
            return &Shaders[Index];
    return nullptr;
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

void FShaderMapResourceCode::Serialize(FArchive& Ar) {
    Ar << ResourceHash;
    Ar << ShaderHashes;
    Ar << ShaderEntries;
    if (!Ar.IsError() && ShaderEntries.Num() != ShaderHashes.Num())
        Ar.SetError(ErrorCode::InvalidData, "FShaderMapResourceCode.ShaderEntries");
}

}
