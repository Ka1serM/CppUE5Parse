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

    UPROPERTY()
    USkeletalMeshComponent* Mesh = nullptr;
    UPROPERTY()
    UCharacterMovementComponent* CharacterMovement = nullptr;
    UPROPERTY()
    UCapsuleComponent* CapsuleComponent = nullptr;

    USkeletalMeshComponent* GetMesh() const { return Mesh; }
    UCharacterMovementComponent* GetCharacterMovement() const { return CharacterMovement; }
    UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }
};

}
