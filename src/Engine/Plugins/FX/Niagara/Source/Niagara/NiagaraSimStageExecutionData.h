#pragma once


#include "NiagaraSimStageExecutionData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraSimStageExecutionLoopData {
    GENERATED_BODY()

    UPROPERTY()
    FName NumLoopsBinding;
    UPROPERTY()
    int32 NumLoops = 1;
    UPROPERTY()
    int32 StartStageIndex = 0;
    UPROPERTY()
    int32 EndStageIndex = 0;
};

}
