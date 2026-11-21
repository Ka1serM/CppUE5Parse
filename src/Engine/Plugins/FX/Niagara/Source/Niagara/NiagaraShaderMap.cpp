#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraShaderMap.h"

namespace ue5parse::runtime {

void FNiagaraShaderMapPointerTable::LoadFromArchive(FArchive& Ar) {
    FShaderMapPointerTable::LoadFromArchive(Ar);
    if (!Ar.IsError())
        Ar << DITypes;
}

void Unfreeze(FMemoryImageReader& Reader, FNiagaraShaderMapId& Id) {
    Id.CompilerVersionID = Reader.Read<FGuid>();
    Id.FeatureLevel = Reader.Read<int32>();
    if (Reader.GetGame() < EGame::GAME_UE5_6)
        Id.BaseScriptID_DEPRECATED = Reader.Read<FGuid>();
    auto ReadString = [](FMemoryImageReader& Element, FString& String) { Element.ReadArray(String.GetCharArray()); };
    Reader.ReadArray(Id.AdditionalDefines, 8u, ReadString);
    Reader.ReadArray(Id.AdditionalVariables, 8u, ReadString);
    Id.BaseCompileHash = Reader.Read<FSHAHash>();
    Reader.ReadArray(Id.ReferencedCompileHashes);
    Id.LayoutParams = Reader.Read<FPlatformTypeLayoutParameters>();
    Reader.ReadArray(Id.ShaderTypeDependencies, 8u, [](FMemoryImageReader& Element, FShaderTypeDependency& Value) {
        Value.ShaderTypeName = FHashedName(Element.Read<uint64>());
        Value.PermutationId = Element.Read<int32>();
        Value.SourceHash = Element.Read<FSHAHash>();
    });
    Id.bUsesRapidIterationParams = Reader.Read<uint8>() != 0;
}

void Unfreeze(FMemoryImageReader& Reader, FNiagaraShaderMapContent& Content) {
    Reader.ReadBase(static_cast<FShaderMapContent&>(Content));
    if (Reader.GetGame() < EGame::GAME_UE5_4) {
        Reader.ReadArray(Content.FriendlyName.GetCharArray());
        Reader.ReadArray(Content.DebugDescription.GetCharArray());
    }
    Reader.ReadStruct(Content.ShaderMapId);
}

FArchive& operator<<(FArchive& Ar, FNiagaraShaderScript& Script) {
    Ar << Script.bLoadedFromCookedMaterial << Script.NumPermutations << Script.BaseCompileHash.DataHash;
    if (!Ar.IsError() && Script.bLoadedFromCookedMaterial) {
        bool Valid = false;
        Ar << Valid;
        if (Valid)
            Script.ShaderMap.emplace().Serialize(Ar);
    }
    return Ar;
}

}
