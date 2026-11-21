#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "NiagaraActor.generated.h"

namespace ue5parse::runtime {

class UNiagaraComponent;

UCLASS()
class ANiagaraActor : public AActor {
    GENERATED_BODY()
public:
    explicit ANiagaraActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UNiagaraComponent> NiagaraComponent = nullptr;
    UPROPERTY()
    uint32 bDestroyOnSystemFinish : 1 = 0;
};

}
