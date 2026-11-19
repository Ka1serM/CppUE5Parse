#include "Engine/Source/Runtime/Engine/Engine/CullDistanceVolume.h"

#include "Engine/Source/Runtime/Engine/Components/BrushComponent.h"

namespace ue5parse::runtime {

ACullDistanceVolume::ACullDistanceVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    GetBrushComponent()->BodyInstance.CollisionProfileName = FName("NoCollision");
    GetBrushComponent()->bAlwaysCreatePhysicsState = true;
    CullDistances.Add(FCullDistanceSizePair{0, 0});
    CullDistances.Add(FCullDistanceSizePair{10000, 0});
}

}
