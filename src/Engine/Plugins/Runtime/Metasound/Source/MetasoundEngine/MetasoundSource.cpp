#include "Engine/Plugins/Runtime/Metasound/Source/MetasoundEngine/MetasoundSource.h"

namespace ue5parse::runtime {

void UMetaSoundSource::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.GetGame() < EGame::GAME_UE5_4)
        return;
    FMetaSoundQualitySettings::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(&CookedQualitySettings));
}

}
