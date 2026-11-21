#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"

#include "NiagaraParameterStore.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraBoundParameter {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraVariableBase Parameter;
    UPROPERTY()
    int32 SrcOffset = 0;
    UPROPERTY()
    int32 DestOffset = 0;
};

}
