#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "CableActor.generated.h"

namespace ue5parse::runtime {

class UCableComponent;

UCLASS()
class ACableActor : public AActor {
    GENERATED_BODY()
public:
    explicit ACableActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UCableComponent> CableComponent = nullptr;
};

}
