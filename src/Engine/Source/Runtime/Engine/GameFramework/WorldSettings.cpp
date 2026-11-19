#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"

namespace ue5parse::runtime {

AWorldSettings::AWorldSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bReplicates = true;
    bAlwaysRelevant = true;
    bReplayRewindable = true;
    DefaultAmbientZoneSettings.bIsWorldSettings = true;
}

}
