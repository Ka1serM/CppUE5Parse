#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "WorldPartitionStreamingPolicy.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FWorldPartitionRuntimeContainerInstance {
    GENERATED_BODY()

    UPROPERTY()
    FGuid ActorGuid;
    UPROPERTY()
    FName ContainerPackage;
};

USTRUCT()
struct FWorldPartitionRuntimeContainer {
    GENERATED_BODY()

    UPROPERTY()
    TMap<FName, FWorldPartitionRuntimeContainerInstance> ContainerInstances;
};

USTRUCT()
struct FWorldPartitionRuntimeContainerResolver {
    GENERATED_BODY()

    UPROPERTY()
    TMap<FName, FWorldPartitionRuntimeContainer> Containers;
    UPROPERTY()
    FName MainContainerPackage;
};

UCLASS(Abstract)
class UWorldPartitionStreamingPolicy : public UObject {
    GENERATED_BODY()
public:
    explicit UWorldPartitionStreamingPolicy(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    bool bShouldMergeStreamingSourceInfo = false;
};

UCLASS()
class UWorldPartitionLevelStreamingPolicy : public UWorldPartitionStreamingPolicy {
    GENERATED_BODY()
public:
    explicit UWorldPartitionLevelStreamingPolicy(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TMap<FName, FName> ActorToCellRemapping;
    UPROPERTY()
    TMap<FName, FName> SubObjectsToCellRemapping;
    UPROPERTY()
    FTopLevelAssetPath SourceWorldAssetPath;
    UPROPERTY()
    FWorldPartitionRuntimeContainerResolver ContainerResolver;
};

}
