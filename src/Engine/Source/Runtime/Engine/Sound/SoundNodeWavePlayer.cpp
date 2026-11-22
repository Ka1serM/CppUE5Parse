#include "Engine/Source/Runtime/Engine/Sound/SoundNodeWavePlayer.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Sound/SoundWave.h"

namespace ue5parse::runtime {

void USoundNodeWavePlayer::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    if (Ar.CustomVerAtLeast(FFrameworkObjectVersion::GUID, FFrameworkObjectVersion::HardSoundReferences, EGame::GAME_UE4_16))
        Ar << SoundWave;
}

}
