#include "Engine/Source/Runtime/Engine/Materials/MaterialFunction.h"

namespace ue5parse::runtime {

void UMaterialFunction::Serialize(FArchive& Ar) {
    if (Ar.GetGame() == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(12);
    Super::Serialize(Ar);
}

}
