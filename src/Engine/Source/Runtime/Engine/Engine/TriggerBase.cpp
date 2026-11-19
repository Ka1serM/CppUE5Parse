#include "Engine/Source/Runtime/Engine/Engine/TriggerBase.h"

#include "Engine/Source/Runtime/Engine/Components/ShapeComponent.h"

namespace ue5parse::runtime {

ATriggerBase::ATriggerBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bHidden = true;
    bCanBeDamaged = false;
    CollisionComponent = ObjectInitializer.CreateDefaultSubobject<UShapeComponent>(FName("CollisionComp"));
    if (CollisionComponent == nullptr)
        return;
    RootComponent = CollisionComponent;
    CollisionComponent->bHiddenInGame = false;
}

}
