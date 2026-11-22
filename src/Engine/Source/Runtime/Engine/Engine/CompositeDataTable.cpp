#include "Engine/Source/Runtime/Engine/Engine/CompositeDataTable.h"

namespace ue5parse::runtime {

void UCompositeDataTable::Serialize(FArchive& Ar) {
    //! Honor of Kings: World writes an array of uint32 before the properties.
    if (Ar.GetGame() == EGame::GAME_HonorofKingsWorld)
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
    Super::Serialize(Ar);
}

}
