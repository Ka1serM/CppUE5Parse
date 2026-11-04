#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/GameFramework/PawnMovementComponent.h"

#include "CharacterMovementComponent.generated.h"

namespace ue5parse::runtime {

class ACharacter;

UCLASS()
class UCharacterMovementComponent : public UPawnMovementComponent {
    GENERATED_BODY()
public:
    explicit UCharacterMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY(Transient, DuplicateTransient)
    TObjectPtr<ACharacter> CharacterOwner = nullptr;
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
    virtual bool CanCrouchInCurrentState() const { return IsFalling() || IsMovingOnGround(); }
    void ApplyNetworkMovementMode(uint8 ReceivedMode);
    void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;
};

}
