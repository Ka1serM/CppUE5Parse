#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"

namespace ue5parse::runtime {

AInfo::AInfo(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryActorTick.bCanEverTick = false;
    bAllowTickBeforeBeginPlay = true;
    bReplicates = false;
    NetUpdateFrequency = 10.0f;
    bHidden = true;
    bReplicateMovement = false;
    bCanBeDamaged = false;
    bEnableAutoLODGeneration = false;
}

}
