#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Pawn.h"

#include "Character.generated.h"

namespace ue5parse::runtime {

class UCapsuleComponent;
class UCharacterMovementComponent;
class USkeletalMeshComponent;

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

    void PostNetReceive() override;
    UPawnMovementComponent* GetMovementComponent() const override;

    USkeletalMeshComponent* GetMesh() const { return Mesh; }
    UCharacterMovementComponent* GetCharacterMovement() const { return CharacterMovement; }
    UCapsuleComponent* GetCapsuleComponent() const { return CapsuleComponent; }
};

}
