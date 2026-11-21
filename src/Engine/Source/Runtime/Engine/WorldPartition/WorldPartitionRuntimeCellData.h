#pragma once

#include "Engine/Source/Runtime/Core/Misc/Optional.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "WorldPartitionRuntimeCellData.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UWorldPartitionRuntimeCellData : public UObject {
    GENERATED_BODY()
public:
    explicit UWorldPartitionRuntimeCellData(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    FBox ContentBounds;
    UPROPERTY()
    TOptional<FBox> CellBounds;
    UPROPERTY()
    FName GridName;
    UPROPERTY()
    int32 Priority = 0;
    UPROPERTY()
    int32 HierarchicalLevel = 0;

    FString DebugName;
};

UCLASS()
class UWorldPartitionRuntimeCellDataSpatialHash : public UWorldPartitionRuntimeCellData {
    GENERATED_BODY()
public:
    explicit UWorldPartitionRuntimeCellDataSpatialHash(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FVector Position = FVector::ZeroVector;
    UPROPERTY()
    float Extent = 0.0f;
    UPROPERTY()
    int32 Level = 0;
};

UCLASS()
class UWorldPartitionRuntimeCellDataHashSet : public UWorldPartitionRuntimeCellData {
    GENERATED_BODY()
public:
    explicit UWorldPartitionRuntimeCellDataHashSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    bool bIs2D = false;
};

}
