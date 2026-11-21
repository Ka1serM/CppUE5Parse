#include "Engine/Source/Runtime/Engine/Sound/SoundNodeWavePlayer.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

void USoundNodeWavePlayer::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || !Ar.CustomVerAtLeast(FFrameworkObjectVersion::GUID, FFrameworkObjectVersion::HardSoundReferences, EGame::GAME_UE4_16))
        return;
    Ar << SoundWave;
    bHasSerializedSoundWave = !Ar.IsError();
}

}
