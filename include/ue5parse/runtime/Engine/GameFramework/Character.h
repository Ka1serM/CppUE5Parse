#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Pawn.h"

#include "Character.generated.h"

namespace ue5parse::runtime {

class UCapsuleComponent;
class UCharacterMovementComponent;
class USkeletalMeshComponent;

// ACharacter: a pawn with a capsule, a skeletal mesh and character movement (UE's GameFramework/Character.h).
UCLASS()
class ACharacter : public APawn {
    GENERATED_BODY()
public:
    explicit ACharacter(const FObjectInitializer& ObjectInitializer);

    static FName MeshComponentName;
    static FName CharacterMovementComponentName;
    static FName CapsuleComponentName;

    UPROPERTY()
    TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;
    UPROPERTY()
    TObjectPtr<UCharacterMovementComponent> CharacterMovement = nullptr;
    UPROPERTY()
    TObjectPtr<UCapsuleComponent> CapsuleComponent = nullptr;

    UPROPERTY(Replicated)
    uint8 ReplicatedMovementMode = 0;

    // A simulated proxy applies the movement mode that the server sent (UE does it in the movement component's
    // SimulatedTick, after PostNetReceive marks the change).
    void PostNetReceive() override;
    UPawnMovementComponent* GetMovementComponent() const override;

    USkeletalMeshComponent* GetMesh() const { return Mesh; }
    UCharacterMovementComponent* GetCharacterMovement() const { return CharacterMovement; }
    UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }
};

}
