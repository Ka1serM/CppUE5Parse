#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraParameterStore.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraScriptExecutionParameterStore.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraScriptRuntimeCompiledData.h"

#include "NiagaraScriptRuntimeCookedData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraScriptRuntimeCookedData {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraScriptRuntimeCompiledData CookedScriptRuntimeCompiledData;
    UPROPERTY()
    FNiagaraScriptExecutionParameterStore CookedExecutionParameterStore;
    UPROPERTY()
    TArray<FNiagaraBoundParameter> CookedRapidIterationBindings;
};

}
