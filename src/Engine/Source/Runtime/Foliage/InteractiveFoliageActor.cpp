#include "Engine/Source/Runtime/Foliage/InteractiveFoliageActor.h"

#include "Engine/Source/Runtime/Engine/Components/CapsuleComponent.h"
#include "Engine/Source/Runtime/Foliage/InteractiveFoliageComponent.h"

namespace ue5parse::runtime {

AInteractiveFoliageActor::AInteractiveFoliageActor(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UInteractiveFoliageComponent>(FName("StaticMeshComponent0"))) {
    StaticMeshComponent->BodyInstance.CollisionProfileName = FName("NoCollision");
    StaticMeshComponent->Mobility = EComponentMobility::Static;
    CapsuleComponent = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(FName("CollisionCylinder"));
    CapsuleComponent->InitCapsuleSize(60.0f, 200.0f);
    CapsuleComponent->BodyInstance.CollisionProfileName = FName("OverlapAllDynamic");
    CapsuleComponent->Mobility = EComponentMobility::Static;
    RootComponent = CapsuleComponent;
    PrimaryActorTick.bCanEverTick = true;
    bCanBeDamaged = true;
    bCollideWhenPlacing = true;
    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
}

}
