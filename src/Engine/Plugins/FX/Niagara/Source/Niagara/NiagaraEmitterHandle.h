#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"

#include "NiagaraEmitterHandle.generated.h"

namespace ue5parse::runtime {

class UNiagaraEmitter;
class UNiagaraStatelessEmitter;

UENUM()
enum class ENiagaraEmitterMode : uint8 { Standard, Stateless };

USTRUCT()
struct FNiagaraEmitterHandle {
    GENERATED_BODY()

    UPROPERTY()
    FGuid Id;
    UPROPERTY()
    FName IdName;
    UPROPERTY()
    bool bIsEnabled = false;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    TObjectPtr<UNiagaraEmitter> Instance = nullptr;
    UPROPERTY()
    FVersionedNiagaraEmitter VersionedInstance;
    UPROPERTY()
    TObjectPtr<UNiagaraStatelessEmitter> StatelessEmitter = nullptr;
    UPROPERTY()
    ENiagaraEmitterMode EmitterMode = ENiagaraEmitterMode::Standard;
};

}
