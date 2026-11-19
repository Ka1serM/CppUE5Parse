#include "Engine/Source/Runtime/Engine/GameFramework/KillZVolume.h"

namespace ue5parse::runtime {

AKillZVolume::AKillZVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bNetLoadOnClient = false;
}

}
