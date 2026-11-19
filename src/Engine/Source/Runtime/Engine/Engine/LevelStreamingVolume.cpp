#include "Engine/Source/Runtime/Engine/Engine/LevelStreamingVolume.h"

#include "Engine/Source/Runtime/Engine/Components/BrushComponent.h"

namespace ue5parse::runtime {

ALevelStreamingVolume::ALevelStreamingVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    GetBrushComponent()->BodyInstance.CollisionProfileName = FName("NoCollision");
    GetBrushComponent()->bAlwaysCreatePhysicsState = true;
    bColored = true;
    BrushColor = FColor(255, 165, 0, 255);
}

}
