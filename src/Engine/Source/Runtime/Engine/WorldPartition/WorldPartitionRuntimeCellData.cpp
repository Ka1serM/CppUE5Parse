#include "Engine/Source/Runtime/Engine/WorldPartition/WorldPartitionRuntimeCellData.h"

namespace ue5parse::runtime {

void UWorldPartitionRuntimeCellData::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetGame() < EGame::GAME_UE5_3)
        return;
    Ar << DebugName;
}

}
