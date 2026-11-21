#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCommon.h"

#include "NiagaraScriptRuntimeCompiledData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraScriptRuntimeCompiledData {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraScriptResolvedDataInterfaceInfo> ResolvedDataInterfaces;
    UPROPERTY()
    TArray<FNiagaraResolvedUserDataInterfaceBinding> ResolvedUserDataInterfaceBindings;
    UPROPERTY()
    TArray<FNiagaraResolvedUObjectInfo> ResolvedUObjects;
    UPROPERTY()
    ENiagaraSimTarget SimTarget = ENiagaraSimTarget::CPUSim;
};

}
