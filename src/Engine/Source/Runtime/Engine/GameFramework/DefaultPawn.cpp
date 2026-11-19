#include "Engine/Source/Runtime/Engine/GameFramework/DefaultPawn.h"

#include "Engine/Source/Runtime/Engine/Components/SphereComponent.h"
#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"
#include "Engine/Source/Runtime/Engine/GameFramework/FloatingPawnMovement.h"

namespace ue5parse::runtime {

ADefaultPawn::ADefaultPawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    BaseEyeHeight = 0.0f;
    bCollideWhenPlacing = false;
    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    CollisionComponent = ObjectInitializer.CreateDefaultSubobject<USphereComponent>(FName("CollisionComponent0"));
    CollisionComponent->SphereRadius = 35.0f;
    CollisionComponent->BodyInstance.CollisionProfileName = FName("Pawn");
    RootComponent = CollisionComponent;

    MovementComponent = Cast<UPawnMovementComponent>(ObjectInitializer.CreateDefaultSubobject(
        ObjectInitializer.GetObj(), FName("MovementComponent0"), UPawnMovementComponent::StaticClass(), UFloatingPawnMovement::StaticClass()));
    MovementComponent->UpdatedComponent = CollisionComponent;

    MeshComponent = ObjectInitializer.CreateOptionalDefaultSubobject<UStaticMeshComponent>(FName("MeshComponent0"));
    if (MeshComponent == nullptr)
        return;
    MeshComponent->SetupAttachment(RootComponent);
    MeshComponent->BodyInstance.CollisionProfileName = FName("Pawn");
    MeshComponent->SetRelativeScale3D_Direct(FVector(35.0f / 160.0f));
}

}
