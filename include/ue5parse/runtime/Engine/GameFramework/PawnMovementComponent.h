#pragma once

#include "ue5parse/runtime/Engine/GameFramework/NavMovementComponent.h"

#include "PawnMovementComponent.generated.h"

namespace ue5parse::runtime {

class APawn;

// UPawnMovementComponent: the movement of a pawn (UE's GameFramework/PawnMovementComponent.h).
UCLASS(Abstract)
class UPawnMovementComponent : public UNavMovementComponent {
    GENERATED_BODY()
public:
    explicit UPawnMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    APawn* PawnOwner = nullptr;

    UFUNCTION(BlueprintCallable)
    APawn* GetPawnOwner() const { return PawnOwner; }
};

}
