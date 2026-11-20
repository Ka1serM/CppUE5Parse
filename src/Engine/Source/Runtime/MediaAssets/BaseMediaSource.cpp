#include "Engine/Source/Runtime/MediaAssets/BaseMediaSource.h"

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

void UBaseMediaSource::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    if (Ar.CustomVerAtLeast(FSequencerObjectVersion::GUID, FSequencerObjectVersion::RenameMediaSourcePlatformPlayers, EGame::GAME_UE4_14)) {
        Ar << PlayerName;
        return;
    }
    FString DummyDefaultPlayer;
    Ar << DummyDefaultPlayer;
}

}
