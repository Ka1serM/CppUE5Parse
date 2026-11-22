#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"

namespace ue5parse::runtime {

AWorldSettings::AWorldSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bReplicates = true;
    bAlwaysRelevant = true;
    bReplayRewindable = true;
    DefaultAmbientZoneSettings.bIsWorldSettings = true;
}

void AWorldSettings::Serialize(FArchive& Ar) {
    if (Ar.GetGame() == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(20);
    Super::Serialize(Ar);
}

}
