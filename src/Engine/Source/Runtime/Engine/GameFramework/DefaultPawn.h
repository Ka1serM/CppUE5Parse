#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Pawn.h"

#include "DefaultPawn.generated.h"

namespace ue5parse::runtime {

class UPawnMovementComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS()
class ADefaultPawn : public APawn {
    GENERATED_BODY()
public:
    explicit ADefaultPawn(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float BaseTurnRate = 45.f;
    UPROPERTY()
    float BaseLookUpRate = 45.f;
    UPROPERTY()
    TObjectPtr<UPawnMovementComponent> MovementComponent = nullptr;
    UPROPERTY()
    TObjectPtr<USphereComponent> CollisionComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> MeshComponent = nullptr;
    UPROPERTY()
    uint8 bAddDefaultMovementBindings : 1 = true;
};

}
