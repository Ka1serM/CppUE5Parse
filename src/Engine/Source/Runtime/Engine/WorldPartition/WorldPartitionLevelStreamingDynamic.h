#pragma once

#include "Engine/Source/Runtime/Engine/Engine/LevelStreamingDynamic.h"

#include "WorldPartitionLevelStreamingDynamic.generated.h"

namespace ue5parse::runtime {

class UWorldPartition;
class UWorldPartitionRuntimeLevelStreamingCell;

UCLASS()
class UWorldPartitionLevelStreamingDynamic : public ULevelStreamingDynamic {
    GENERATED_BODY()
public:
    explicit UWorldPartitionLevelStreamingDynamic(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    bool bShouldBeAlwaysLoaded = false;
    UPROPERTY()
    bool bHasSetLevelTransform = false;
    UPROPERTY()
    TWeakObjectPtr<UWorldPartitionRuntimeLevelStreamingCell> StreamingCell;
    //! UE 5.0 to 5.3 store this as an object reference, so the soft property also reads ObjectProperty tags.
    UPROPERTY()
    TSoftObjectPtr<UWorldPartition> OuterWorldPartition;
};

}
