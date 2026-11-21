#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraUserRedirectionParameterStore.h"

#include "NiagaraScriptExecutionParameterStore.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraScriptExecutionPaddingInfo {
    GENERATED_BODY()

    UPROPERTY()
    uint16 SrcOffset = 0;
    UPROPERTY()
    uint16 DestOffset = 0;
    UPROPERTY()
    uint16 SrcSize = 0;
    UPROPERTY()
    uint16 DestSize = 0;
};

USTRUCT()
struct FNiagaraScriptExecutionParameterStore : public FNiagaraParameterStore {
    GENERATED_BODY()

    UPROPERTY()
    int32 ParameterSize = 0;
    UPROPERTY()
    uint32 PaddedParameterSize = 0;
    UPROPERTY()
    TArray<FNiagaraScriptExecutionPaddingInfo> PaddingInfo;
    UPROPERTY()
    uint8 bInitialized : 1 = 0;
};

}
