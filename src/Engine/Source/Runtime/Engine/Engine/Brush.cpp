#include "Engine/Source/Runtime/Engine/Engine/Brush.h"

#include "Engine/Source/Runtime/Engine/Components/BrushComponent.h"

namespace ue5parse::runtime {

ABrush::ABrush(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    BrushComponent = ObjectInitializer.CreateDefaultSubobject<UBrushComponent>(FName("BrushComponent0"));
    BrushComponent->Mobility = EComponentMobility::Static;
    BrushComponent->bGenerateOverlapEvents = false;
    BrushComponent->bCanEverAffectNavigation = false;
    RootComponent = BrushComponent;
    bHidden = true;
    bCanBeDamaged = false;
    bCollideWhenPlacing = true;
    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
}

}
