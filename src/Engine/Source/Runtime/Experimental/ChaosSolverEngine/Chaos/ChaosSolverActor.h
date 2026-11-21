#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "ChaosSolverActor.generated.h"

namespace ue5parse::runtime {

class UBillboardComponent;
class UChaosGameplayEventDispatcher;

UCLASS()
class AChaosSolverActor : public AActor {
    GENERATED_BODY()
public:
    explicit AChaosSolverActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UChaosGameplayEventDispatcher> GameplayEventDispatcherComponent = nullptr;
};

}
