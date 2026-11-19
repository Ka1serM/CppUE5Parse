#include "Engine/Source/Runtime/Engine/Engine/NavigationObjectBase.h"

#include "Engine/Source/Runtime/Engine/Components/CapsuleComponent.h"

namespace ue5parse::runtime {

ANavigationObjectBase::ANavigationObjectBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    CapsuleComponent = ObjectInitializer.CreateDefaultSubobject<UCapsuleComponent>(FName("CollisionCapsule"));
    CapsuleComponent->ShapeColor = FColor(255, 138, 5, 255);
    CapsuleComponent->bDrawOnlyIfSelected = true;
    CapsuleComponent->InitCapsuleSize(50.0f, 50.0f);
    CapsuleComponent->BodyInstance.CollisionProfileName = FName("NoCollision");
    CapsuleComponent->bShouldCollideWhenPlacing = true;
    CapsuleComponent->bShouldUpdatePhysicsVolume = true;
    CapsuleComponent->Mobility = EComponentMobility::Static;
    RootComponent = CapsuleComponent;
    bCollideWhenPlacing = true;
    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
}

}
