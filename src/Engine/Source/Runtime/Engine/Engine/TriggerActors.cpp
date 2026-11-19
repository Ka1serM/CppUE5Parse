#include "Engine/Source/Runtime/Engine/Components/BoxComponent.h"
#include "Engine/Source/Runtime/Engine/Components/CapsuleComponent.h"
#include "Engine/Source/Runtime/Engine/Components/SphereComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/TriggerBox.h"
#include "Engine/Source/Runtime/Engine/Engine/TriggerCapsule.h"
#include "Engine/Source/Runtime/Engine/Engine/TriggerSphere.h"

namespace ue5parse::runtime {

namespace {

const FColor TriggerBaseColor(100, 255, 100, 255);
const FName TriggerCollisionProfileName("Trigger");

}

ATriggerCapsule::ATriggerCapsule(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UCapsuleComponent>(FName("CollisionComp"))) {
    auto* CapsuleCollisionComponent = CastChecked<UCapsuleComponent>(GetCollisionComponent());
    CapsuleCollisionComponent->ShapeColor = TriggerBaseColor;
    CapsuleCollisionComponent->InitCapsuleSize(40.0f, 80.0f);
    CapsuleCollisionComponent->BodyInstance.CollisionProfileName = TriggerCollisionProfileName;
    bCollideWhenPlacing = true;
    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
}

ATriggerBox::ATriggerBox(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer.SetDefaultSubobjectClass<UBoxComponent>(FName("CollisionComp"))) {
    auto* BoxCollisionComponent = CastChecked<UBoxComponent>(GetCollisionComponent());
    BoxCollisionComponent->ShapeColor = TriggerBaseColor;
    BoxCollisionComponent->BoxExtent = FVector(40.0f, 40.0f, 40.0f);
    BoxCollisionComponent->BodyInstance.CollisionProfileName = TriggerCollisionProfileName;
}

ATriggerSphere::ATriggerSphere(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<USphereComponent>(FName("CollisionComp"))) {
    auto* SphereCollisionComponent = CastChecked<USphereComponent>(GetCollisionComponent());
    SphereCollisionComponent->ShapeColor = TriggerBaseColor;
    SphereCollisionComponent->SphereRadius = 40.0f;
    SphereCollisionComponent->BodyInstance.CollisionProfileName = TriggerCollisionProfileName;
}

}
