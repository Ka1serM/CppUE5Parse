#include "Engine/Source/Runtime/Engine/Engine/WorldComposition.h"

#include "Engine/Source/Runtime/Engine/Engine/LevelStreaming.h"

namespace ue5parse::runtime {

void UWorldComposition::Serialize(FArchive& Ar) {
    if (Ar.GetGame() == EGame::GAME_WorldofJadeDynasty)
        Ar.SkipBytes(24);
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetRemaining() <= 0)
        return;
    Ar << WorldRoot;
    Ar << Tiles;
    TArray<UObject*> Streaming;
    Ar << Streaming;
    TilesStreaming.Empty(Streaming.Num());
    for (UObject* Level : Streaming)
        TilesStreaming.Add(Cast<ULevelStreaming>(Level));
}

}
