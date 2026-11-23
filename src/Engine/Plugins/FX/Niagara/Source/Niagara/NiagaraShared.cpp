#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraShared.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

bool FNiagaraDataInterfaceGeneratedFunction::Serialize(FArchive& Ar) {
    Ar << DefinitionName << InstanceName << Specifiers;
    const bool bGameHasVariadicParameters = Ar.GetGame() == EGame::GAME_DuetNightAbyss || Ar.GetGame() == EGame::GAME_HonorofKingsWorld;
    if (Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::AddVariadicParametersToGPUFunctionInfo, EGame::GAME_UE5_2)
        || bGameHasVariadicParameters)
        Ar << VariadicInputs << VariadicOutputs;
    if (Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::SerializeUsageBitMaskToGPUFunctionInfo, EGame::GAME_UE5_6))
        Ar << MiscUsageBitMask;
    return true;
}

bool FNiagaraDataInterfaceGPUParamInfo::Serialize(FArchive& Ar) {
    Ar << DataInterfaceHLSLSymbol << DIClassName;
    if (Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::AddGeneratedFunctionsToGPUParamInfo, EGame::GAME_UE4_25))
        Ar << GeneratedFunctions;
    return true;
}

void FNiagaraShaderScript::SerializeShaderMap(FArchive& Ar) {
    const EGame Game = Ar.GetGame();
    const bool bGameHasBaseCompileHash = Game == EGame::GAME_DuetNightAbyss || Game == EGame::GAME_HonorofKingsWorld;
    bool bCooked = false;
    Ar << bCooked;
    if (Game >= EGame::GAME_UE4_26)
        Ar << NumPermutations;
    if (Game >= EGame::GAME_UE4_26 && Game < EGame::GAME_UE5_0)
        Ar << ShaderStageToPermutation;
    if (Game >= EGame::GAME_UE5_2 || bGameHasBaseCompileHash)
        Ar << BaseCompileHash;
    bLoadedFromCookedMaterial = bCooked;
    if (Ar.IsError() || !bCooked)
        return;
    bool bValid = false;
    Ar << bValid;
    if (Ar.IsError() || !bValid)
        return;
    GameThreadShaderMap.emplace().Serialize(Ar);
}

void Unfreeze(FMemoryImageReader& Reader, FNiagaraShaderMapId& Value) {
    const EGame Game = Reader.GetGame();
    const bool bHasVariablesAndDependencies = Game >= EGame::GAME_UE4_27 || Game == EGame::GAME_HonorofKingsWorld;
    Value.CompilerVersionID = Reader.Read<FGuid>();
    Value.FeatureLevel = Reader.Read<ERHIFeatureLevel::Type>();
    if (Game < EGame::GAME_UE5_6)
        Value.BaseScriptID_DEPRECATED = Reader.Read<FGuid>();
    auto ReadString = [](FMemoryImageReader& Element, FString& String) { String = Element.ReadMemoryImageString(); };
    Reader.ReadArray(Value.AdditionalDefines, 8u, ReadString);
    if (bHasVariablesAndDependencies)
        Reader.ReadArray(Value.AdditionalVariables, 8u, ReadString);
    Value.BaseCompileHash = Reader.Read<FSHAHash>();
    Reader.ReadArray(Value.ReferencedCompileHashes);
    Value.LayoutParams = Reader.Read<FPlatformTypeLayoutParameters>();
    if (bHasVariablesAndDependencies)
        Reader.ReadArray(Value.ShaderTypeDependencies);
    Value.bUsesRapidIterationParams = Reader.Read<uint8>() != 0u;
}

void Unfreeze(FMemoryImageReader& Reader, FNiagaraShaderMapContent& Value) {
    Reader.ReadBase(static_cast<FShaderMapContent&>(Value));
    if (Reader.GetGame() < EGame::GAME_UE5_4) {
        Value.FriendlyName = Reader.ReadMemoryImageString();
        Value.DebugDescription = Reader.ReadMemoryImageString();
    }
    Reader.ReadStruct(Value.ShaderMapId);
}

}
