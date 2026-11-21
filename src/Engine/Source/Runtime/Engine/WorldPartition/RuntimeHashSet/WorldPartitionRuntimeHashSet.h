#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Engine/WorldPartition/RuntimeHashSet/RuntimePartition.h"
#include "Engine/Source/Runtime/Engine/WorldPartition/WorldPartitionRuntimeCell.h"
#include "Engine/Source/Runtime/Engine/WorldPartition/WorldPartitionRuntimeHash.h"

#include "WorldPartitionRuntimeHashSet.generated.h"

namespace ue5parse::runtime {

class UHLODLayer;

USTRUCT()
struct FRuntimePartitionHLODSetup {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    TObjectPtr<URuntimePartition> PartitionLayer = nullptr;
    UPROPERTY()
    bool bIsSpatiallyLoaded = true;
};

USTRUCT()
struct FRuntimePartitionDesc {
    GENERATED_BODY()

    UPROPERTY()
    TSubclassOf<URuntimePartition> Class;
    UPROPERTY()
    FName Name;
    UPROPERTY()
    TObjectPtr<URuntimePartition> MainLayer = nullptr;
    UPROPERTY()
    TArray<FRuntimePartitionHLODSetup> HLODSetups;
};

USTRUCT()
struct FRuntimePartitionStreamingData {
    GENERATED_BODY()

    UPROPERTY()
    FName Name;
    UPROPERTY()
    int32 LoadingRange = 0;
    UPROPERTY()
    TArray<TObjectPtr<UWorldPartitionRuntimeCell>> StreamingCells;
    UPROPERTY()
    TArray<TObjectPtr<UWorldPartitionRuntimeCell>> NonStreamingCells;
    UPROPERTY()
    TArray<TObjectPtr<UWorldPartitionRuntimeCell>> SpatiallyLoadedCells;
    UPROPERTY()
    TArray<TObjectPtr<UWorldPartitionRuntimeCell>> NonSpatiallyLoadedCells;
};

UCLASS()
class UWorldPartitionRuntimeHashSet : public UWorldPartitionRuntimeHash {
    GENERATED_BODY()
public:
    explicit UWorldPartitionRuntimeHashSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FRuntimePartitionDesc PersistentPartitionDesc;
    UPROPERTY()
    TArray<FRuntimePartitionDesc> RuntimePartitions;
    UPROPERTY()
    TArray<TObjectPtr<UWorldPartitionRuntimeCell>> NonSpatiallyLoadedRuntimeCells;
    UPROPERTY()
    TArray<FRuntimePartitionStreamingData> RuntimeStreamingData;
    UPROPERTY()
    TMap<FGuid, FGuid> StandaloneHLODActorToSourceCellsMap;
    UPROPERTY()
    TMap<FGuid, FGuid> CustomHLODActorToSourceCellsMap;
};

}
