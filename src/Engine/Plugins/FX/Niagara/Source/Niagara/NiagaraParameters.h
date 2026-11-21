#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"

#include "NiagaraParameters.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraParameters {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraVariable> Parameters;
};

}
