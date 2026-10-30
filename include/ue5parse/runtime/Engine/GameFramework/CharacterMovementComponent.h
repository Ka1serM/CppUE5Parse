#pragma once

#include "ue5parse/runtime/Engine/Engine/EngineTypes.h"
#include "ue5parse/runtime/Engine/GameFramework/PawnMovementComponent.h"

#include "CharacterMovementComponent.generated.h"

namespace ue5parse::runtime {

class ACharacter;

// UCharacterMovementComponent: the walking, falling, swimming and flying movement of a character (UE's
// GameFramework/CharacterMovementComponent.h).
UCLASS()
class UCharacterMovementComponent : public UPawnMovementComponent {
    GENERATED_BODY()
public:
    explicit UCharacterMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    ACharacter* CharacterOwner = nullptr;
    UPROPERTY()
    TEnumAsByte<EMovementMode> MovementMode = MOVE_None;
    UPROPERTY()
    uint8 CustomMovementMode = 0;
    UPROPERTY()
    float MaxWalkSpeed = 600.0f;
    UPROPERTY()
    FVector Acceleration = FVector::ZeroVector;

    UFUNCTION(BlueprintCallable)
    FVector GetCurrentAcceleration() const { return Acceleration; }
    UFUNCTION(BlueprintCallable)
    bool IsFalling() const { return MovementMode == MOVE_Falling && UpdatedComponent != nullptr; }
    UFUNCTION(BlueprintCallable)
    bool IsMovingOnGround() const { return (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking) && UpdatedComponent != nullptr; }
    UFUNCTION(BlueprintCallable)
    bool IsFlying() const { return MovementMode == MOVE_Flying && UpdatedComponent != nullptr; }
    // ACharacter::OnRep_ReplicatedMovement's part of a simulated proxy: the movement mode that the server sent (UE's
    // ApplyNetworkMovementMode, without custom modes' packing beyond the low bits).
    void ApplyNetworkMovementMode(uint8 ReceivedMode);
};

}
