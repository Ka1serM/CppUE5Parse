#include "Engine/Source/Runtime/AugmentedReality/AROriginActor.h"

namespace ue5parse::runtime {

AAROriginActor::AAROriginActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryActorTick.bCanEverTick = false;
    bAllowTickBeforeBeginPlay = false;
    bReplicates = false;
    bReplicateMovement = false;
    bCanBeDamaged = false;
}

}
