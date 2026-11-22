#include "Engine/Source/Runtime/Engine/Engine/StaticMeshSocket.h"

namespace ue5parse::runtime {

void UStaticMeshSocket::Serialize(FArchive& Ar) {
    if (Ar.GetGame() == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(16);
    Super::Serialize(Ar);
}

}
