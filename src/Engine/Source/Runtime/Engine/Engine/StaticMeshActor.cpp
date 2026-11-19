#include "Engine/Source/Runtime/Engine/Engine/StaticMeshActor.h"

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

namespace ue5parse::runtime {

AStaticMeshActor::AStaticMeshActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bCanBeDamaged = false;
    StaticMeshComponent = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(FName("StaticMeshComponent0"));
    StaticMeshComponent->BodyInstance.CollisionProfileName = FName("BlockAll");
    StaticMeshComponent->Mobility = EComponentMobility::Static;
    StaticMeshComponent->bGenerateOverlapEvents = false;
    StaticMeshComponent->bUseDefaultCollision = true;
    RootComponent = StaticMeshComponent;
    bCanBeInCluster = ObjectInitializer.GetClass() == StaticClass();
}

}
