#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/PawnMovementComponent.h"

#include "FloatingPawnMovement.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UFloatingPawnMovement : public UPawnMovementComponent {
    GENERATED_BODY()
public:
    explicit UFloatingPawnMovement(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    float MaxSpeed = 1200.f;
    UPROPERTY()
    float Acceleration = 4000.f;
    UPROPERTY()
    float Deceleration = 8000.f;
    UPROPERTY()
    float TurningBoost = 8.0f;
};

}
