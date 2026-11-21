#include "Engine/Plugins/NetcodeUnitTest/NetcodeUnitTest/Source/NetcodeUnitTest/NUTActor.h"

namespace ue5parse::runtime {

ANUTActor::ANUTActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    bAlwaysRelevant = true;
    bReplicateMovement = false;
    NetUpdateFrequency = 1.0f;
}

}
