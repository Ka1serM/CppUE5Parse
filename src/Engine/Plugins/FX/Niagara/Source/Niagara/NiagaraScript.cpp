#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraScript.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

bool FNiagaraVMExecutableData::IsValid() const {
    return LastCompileStatus != ENiagaraScriptCompileStatus::NCS_Unknown;
}

void UNiagaraScript::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetGame() < EGame::GAME_UE4_25)
        return;
    if (!Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::MovedToDerivedDataCache, EGame::GAME_UE4_20))
        return;
    if (Usage != ENiagaraScriptUsage::ParticleGPUComputeScript || !CachedScriptVM.IsValid())
        return;
    //! Before UE 5.8 a cooked game asks each data interface class of the script whether it runs on the GPU, which needs the class code.
    if (Ar.GetGame() < EGame::GAME_UE5_8 && !CachedScriptVM.DataInterfaceInfo.IsEmpty())
        return;
    SerializeNiagaraShaderMaps(Ar);
}

void UNiagaraScript::SerializeNiagaraShaderMaps(FArchive& Ar) {
    const int32 ResourceCount = Ar.ReadCount(static_cast<int32>(sizeof(uint32)));
    LoadedScriptResources.Empty(ResourceCount);
    for (int32 ResourceIt = 0; ResourceIt < ResourceCount && !Ar.IsError(); ResourceIt++)
        LoadedScriptResources[LoadedScriptResources.AddDefaulted()].SerializeShaderMap(Ar);
}

bool FNiagaraVMExecutableByteCode::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("ArrayProperty"))
        return false;
    Ar << Data;
    return true;
}

}
