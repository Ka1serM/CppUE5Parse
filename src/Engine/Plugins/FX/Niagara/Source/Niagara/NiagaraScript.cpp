#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraScript.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

void UNiagaraScript::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetGame() < EGame::GAME_UE5_3 || Usage != ENiagaraScriptUsage::ParticleGPUComputeScript
        || CachedScriptVM.LastCompileStatus == ENiagaraScriptCompileStatus::NCS_Unknown)
        return;
    if (!Ar.CustomVerAtLeast(FNiagaraCustomVersion::GUID, FNiagaraCustomVersion::MovedToDerivedDataCache, EGame::GAME_UE5_3))
        return;
    //! Before UE 5.8 each data interface class must implement CanExecuteOnTarget for GPU scripts.
    if (Ar.GetGame() < EGame::GAME_UE5_8 && !CachedScriptVM.DataInterfaceInfo.IsEmpty())
        return;
    Ar << LoadedScriptResources;
}

bool FNiagaraVMExecutableByteCode::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("ArrayProperty"))
        return false;
    Ar << Data;
    return true;
}

}
