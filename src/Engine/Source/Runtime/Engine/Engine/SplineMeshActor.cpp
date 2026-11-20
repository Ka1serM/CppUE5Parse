#include "Engine/Source/Runtime/Engine/Engine/SplineMeshActor.h"

#include "Engine/Source/Runtime/Engine/Components/SplineMeshComponent.h"

namespace ue5parse::runtime {

ASplineMeshActor::ASplineMeshActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bCanBeDamaged = false;
    SplineMeshComponent = ObjectInitializer.CreateDefaultSubobject<USplineMeshComponent>(FName("SplineMeshComponent0"));
    SplineMeshComponent->BodyInstance.CollisionProfileName = FName("BlockAll");
    SplineMeshComponent->Mobility = EComponentMobility::Static;
    SplineMeshComponent->bGenerateOverlapEvents = false;
    SplineMeshComponent->bAllowSplineEditingPerInstance = true;
    RootComponent = SplineMeshComponent;
}

}
