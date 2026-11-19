#include "Engine/Source/Runtime/Engine/Engine/PostProcessVolume.h"

#include "Engine/Source/Runtime/Engine/Components/BrushComponent.h"

namespace ue5parse::runtime {

APostProcessVolume::APostProcessVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    GetBrushComponent()->BodyInstance.CollisionProfileName = FName("NoCollision");
    GetBrushComponent()->bAlwaysCreatePhysicsState = true;
    GetBrushComponent()->Mobility = EComponentMobility::Movable;
}

}
