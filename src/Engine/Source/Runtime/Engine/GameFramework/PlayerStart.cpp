#include "Engine/Source/Runtime/Engine/GameFramework/PlayerStart.h"

#include "Engine/Source/Runtime/Engine/Components/CapsuleComponent.h"

namespace ue5parse::runtime {

APlayerStart::APlayerStart(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    GetCapsuleComponent()->InitCapsuleSize(40.0f, 92.0f);
    GetCapsuleComponent()->bShouldUpdatePhysicsVolume = false;
}

}
