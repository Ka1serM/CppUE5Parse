#pragma once

#include "ue5parse/runtime/Engine/Components/ActorComponent.h"
#include "ue5parse/runtime/Engine/Components/SceneComponent.h"

#include "MovementComponent.generated.h"

namespace ue5parse::runtime {

// UMovementComponent: what moves an updated component, with its velocity (UE's GameFramework/MovementComponent.h).
UCLASS(Abstract)
class UMovementComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<USceneComponent> UpdatedComponent = nullptr;
    UPROPERTY()
    FVector Velocity = FVector::ZeroVector;
};

}
