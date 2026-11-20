#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "VolumetricLightmapGrid.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FVolumetricLightMapGridCell {
    GENERATED_BODY()
    UPROPERTY()
    FBox Bounds;
    FByteBulkData BulkData;
    UPROPERTY()
    uint32 CellID = static_cast<uint32>(-1);
};

USTRUCT()
struct FVolumetricLightMapGridDesc {
    GENERATED_BODY()
    UPROPERTY()
    FBox GridBounds;
    UPROPERTY()
    FGuid Guid;
    UPROPERTY()
    float CellSize = 0.0f;
    UPROPERTY()
    float DetailCellSize = 0.0f;
    UPROPERTY()
    float BrickSize = 0.0f;
    UPROPERTY()
    TArray<FVolumetricLightMapGridCell> Cells;

    void SerializeBulkData(FArchive& Ar);
};

}
