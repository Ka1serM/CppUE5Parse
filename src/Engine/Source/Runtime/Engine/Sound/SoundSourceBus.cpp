#include "Engine/Source/Runtime/Engine/Sound/SoundSourceBus.h"

namespace ue5parse::runtime {

void USoundSourceBus::SerializeCuePoints(FArchive&, bool) {}

void USoundSourceBus::SerializeCookedPlatformData(FArchive& Ar) {
    //! Since 5.3 a source bus saves no streamed chunks.
    if (Ar.GetGame() < EGame::GAME_UE5_3)
        Super::SerializeCookedPlatformData(Ar);
}

}
