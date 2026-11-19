#include "Engine/Source/Runtime/Engine/GameFramework/PhysicsVolume.h"

#include "Engine/Source/Runtime/Engine/Components/BrushComponent.h"

namespace ue5parse::runtime {

APhysicsVolume::APhysicsVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    GetBrushComponent()->BodyInstance.CollisionProfileName = FName("OverlapAllDynamic");
    bAlwaysRelevant = true;
    NetUpdateFrequency = 0.1f;
    bReplicateMovement = false;
}

}
