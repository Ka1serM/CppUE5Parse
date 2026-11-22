#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Info.h"
#include "Engine/Source/Runtime/Engine/WorldPartition/WorldPartitionRuntimeCell.h"
#include "Engine/Source/Runtime/Engine/WorldPartition/WorldPartitionRuntimeHash.h"

#include "WorldPartitionRuntimeSpatialHash.generated.h"

namespace ue5parse::runtime {

class UHLODLayer;

USTRUCT()
struct FSpatialHashRuntimeGrid {
    GENERATED_BODY()
    UPROPERTY()
    FName GridName;
    UPROPERTY()
    int32 CellSize = 12800;
    UPROPERTY()
    float LoadingRange = 25600.0f;
    UPROPERTY()
    bool bBlockOnSlowStreaming = false;
    UPROPERTY()
    FVector2D Origin = FVector2D::ZeroVector;
    UPROPERTY()
    int32 Priority = 0;
    UPROPERTY()
    FLinearColor DebugColor;
    UPROPERTY()
    bool bClientOnlyVisible = false;
    UPROPERTY()
    TObjectPtr<UHLODLayer> HLODLayer = nullptr;
};

UCLASS()
class ASpatialHashRuntimeGridInfo : public AInfo {
    GENERATED_BODY()
public:
    explicit ASpatialHashRuntimeGridInfo(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FSpatialHashRuntimeGrid GridSettings;
};

USTRUCT()
struct FSpatialHashStreamingGridLayerCell {
    GENERATED_BODY()

    UPROPERTY()
    TArray<TObjectPtr<UWorldPartitionRuntimeCell>> GridCells;
};

USTRUCT()
struct FSpatialHashStreamingGridLevel {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FSpatialHashStreamingGridLayerCell> LayerCells;
    UPROPERTY()
    TMap<int64, int32> LayerCellsMapping;
};

USTRUCT()
struct FSpatialHashStreamingGrid {
    GENERATED_BODY()

    UPROPERTY()
    FName GridName;
    UPROPERTY()
    FVector Origin = FVector::ZeroVector;
    UPROPERTY()
    int32 CellSize = 0;
    UPROPERTY()
    float LoadingRange = 0.0f;
    UPROPERTY()
    bool bBlockOnSlowStreaming = false;
    UPROPERTY()
    FLinearColor DebugColor;
    UPROPERTY()
    TArray<FSpatialHashStreamingGridLevel> GridLevels;
    UPROPERTY()
    FBox WorldBounds;
    UPROPERTY()
    bool bClientOnlyVisible = false;
    UPROPERTY()
    TObjectPtr<UHLODLayer> HLODLayer = nullptr;
    UPROPERTY()
    int32 GridIndex = INDEX_NONE;
};

UCLASS()
class UWorldPartitionRuntimeSpatialHash : public UWorldPartitionRuntimeHash {
    GENERATED_BODY()
public:
    explicit UWorldPartitionRuntimeSpatialHash(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TArray<FSpatialHashRuntimeGrid> Grids;
    UPROPERTY()
    TArray<FSpatialHashStreamingGrid> StreamingGrids;
    UPROPERTY()
    bool bEnableZCulling = false;
};

}
