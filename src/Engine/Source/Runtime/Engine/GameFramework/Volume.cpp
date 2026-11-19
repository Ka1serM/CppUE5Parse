#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "Engine/Source/Runtime/Engine/Components/BrushComponent.h"

namespace ue5parse::runtime {

AVolume::AVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    GetBrushComponent()->AlwaysLoadOnClient = true;
    GetBrushComponent()->AlwaysLoadOnServer = true;
    GetBrushComponent()->BodyInstance.CollisionProfileName = FName("OverlapAll");
    GetBrushComponent()->bGenerateOverlapEvents = true;
    bReplicateMovement = false;
    bCanBeDamaged = false;
}

}
