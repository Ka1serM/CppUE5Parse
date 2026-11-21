#pragma once

#include "Engine/Source/Runtime/Engine/WorldPartition/DataLayer/DataLayerInstanceNames.h"
#include "Engine/Source/Runtime/Engine/WorldPartition/WorldPartitionRuntimeCellData.h"

#include "WorldPartitionRuntimeCell.generated.h"

namespace ue5parse::runtime {

class UWorldPartitionLevelStreamingDynamic;

USTRUCT()
struct FWorldPartitionRuntimeCellDebugInfo {
    GENERATED_BODY()

    UPROPERTY()
    FString Name;
    UPROPERTY()
    FName GridName;
    UPROPERTY()
    int64 CoordX = 0;
    UPROPERTY()
    int64 CoordY = 0;
    UPROPERTY()
    int64 CoordZ = 0;
};

UCLASS(Abstract)
class UWorldPartitionRuntimeCell : public UObject {
    GENERATED_BODY()
public:
    explicit UWorldPartitionRuntimeCell(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    bool bIsAlwaysLoaded = false;
    UPROPERTY()
    bool bIsSpatiallyLoaded = false;
    UPROPERTY()
    bool bIsCustomHLODPlaceholderCell = false;
    UPROPERTY()
    FDataLayerInstanceNames DataLayers;
    UPROPERTY()
    FIntVector Coords;
    UPROPERTY()
    FName GridName;
    UPROPERTY()
    FString DebugName;
    UPROPERTY()
    FBox ContentBounds;
    UPROPERTY()
    int32 Priority = 0;
    UPROPERTY()
    bool bClientOnlyVisible = false;
    UPROPERTY()
    bool bIsHLOD = false;
    UPROPERTY()
    bool bBlockOnSlowLoading = false;
    UPROPERTY()
    FWorldPartitionRuntimeCellDebugInfo DebugInfo;
    UPROPERTY()
    FGuid ContentBundleID;
    UPROPERTY()
    FGuid CellGuid;
    UPROPERTY()
    FGuid SourceCellGuid;
    UPROPERTY()
    TObjectPtr<UWorldPartitionRuntimeCellData> RuntimeCellData = nullptr;
    UPROPERTY()
    FLinearColor CellDebugColor = FLinearColor::Black;
};

//! UE 5.0 and 5.1 derive the level streaming cell from this class and cook its grid props into every cell.
UCLASS(Abstract)
class UWorldPartitionRuntimeSpatialHashCell : public UWorldPartitionRuntimeCell {
    GENERATED_BODY()
public:
    explicit UWorldPartitionRuntimeSpatialHashCell(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FVector Position = FVector::ZeroVector;
    UPROPERTY()
    float Extent = 0.0f;
    UPROPERTY()
    int32 Level = 0;
};

UCLASS()
class UWorldPartitionRuntimeLevelStreamingCell : public UWorldPartitionRuntimeSpatialHashCell {
    GENERATED_BODY()
public:
    explicit UWorldPartitionRuntimeLevelStreamingCell(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UWorldPartitionLevelStreamingDynamic> LevelStreaming = nullptr;
};

}
