#include "Engine/Source/Runtime/Engine/WorldPartition/StaticLightingData/VolumetricLightmapGrid.h"

namespace ue5parse::runtime {

void FVolumetricLightMapGridDesc::SerializeBulkData(FArchive& Ar) {
    for (FVolumetricLightMapGridCell& Cell : Cells)
        Cell.BulkData.Serialize(Ar);
}

}
