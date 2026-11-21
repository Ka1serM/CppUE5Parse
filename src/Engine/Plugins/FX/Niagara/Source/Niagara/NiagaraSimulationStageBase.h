#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraMergeable.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraSimulationStageBase.generated.h"

namespace ue5parse::runtime {

class UNiagaraScript;

UCLASS()
class UNiagaraSimulationStageBase : public UNiagaraMergeable {
    GENERATED_BODY()
public:
    explicit UNiagaraSimulationStageBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UNiagaraScript> Script = nullptr;
    UPROPERTY()
    FName SimulationStageName;
    UPROPERTY()
    uint32 bEnabled : 1 = 0;
};

}
