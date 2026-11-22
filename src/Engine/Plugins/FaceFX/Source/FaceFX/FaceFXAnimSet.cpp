#include "Engine/Plugins/FaceFX/Source/FaceFX/FaceFXAnimSet.h"

namespace ue5parse::runtime {

void UFaceFXAnimSet::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    //! Borderlands 4 keeps its FaceFX data in properties.
    if (Ar.IsError() || Ar.GetGame() == EGame::GAME_Borderlands4)
        return;
    Ar << RawFaceFXAnimSetBytes << RawFaceFXMiniSessionBytes;
}

}
