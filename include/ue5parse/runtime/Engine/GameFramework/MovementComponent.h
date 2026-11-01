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
    explicit UMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) { bWantsInitializeComponent = true; }

    // Sets the updated component. Not ported: the owner's root as the updated component when it has none
    // (bAutoRegisterUpdatedComponent).
    void InitializeComponent() override;
    virtual void SetUpdatedComponent(USceneComponent* NewUpdatedComponent);

    UPROPERTY(Transient, DuplicateTransient)
    TObjectPtr<USceneComponent> UpdatedComponent = nullptr;
    UPROPERTY()
    FVector Velocity = FVector::ZeroVector;
};

}
