#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/Engine/WorldPartition/WorldPartitionRuntimeHash.h"
#include "Engine/Source/Runtime/Engine/WorldPartition/WorldPartitionStreamingPolicy.h"

#include "WorldPartition.generated.h"

namespace ue5parse::runtime {

class UHLODLayer;

UENUM()
enum class EWorldPartitionServerStreamingMode : uint8 {
    ProjectDefault = 0,
    Disabled = 1,
    Enabled = 2,
    EnabledInPIE = 3,
};

UENUM()
enum class EWorldPartitionServerStreamingOutMode : uint8 {
    ProjectDefault = 0,
    Disabled = 1,
    Enabled = 2,
};

UENUM()
enum class EWorldPartitionDataLayersLogicOperator : uint8 {
    Or,
    And,
};

UCLASS()
class UActorDescContainer : public UObject {
    GENERATED_BODY()
public:
    explicit UActorDescContainer(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

UCLASS()
class UWorldPartition : public UObject {
    GENERATED_BODY()
public:
    explicit UWorldPartition(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    TObjectPtr<UActorDescContainer> ActorDescContainer = nullptr;
    UPROPERTY()
    TSubclassOf<UWorldPartitionStreamingPolicy> WorldPartitionStreamingPolicyClass;
    UPROPERTY()
    TObjectPtr<UWorldPartitionRuntimeHash> RuntimeHash = nullptr;
    UPROPERTY()
    bool bEnableStreaming = true;
    UPROPERTY()
    bool bStreamingWasEnabled = true;
    UPROPERTY()
    bool bCanBeUsedByLevelInstance = false;
    UPROPERTY()
    EWorldPartitionServerStreamingMode ServerStreamingMode = EWorldPartitionServerStreamingMode::ProjectDefault;
    UPROPERTY()
    EWorldPartitionServerStreamingOutMode ServerStreamingOutMode = EWorldPartitionServerStreamingOutMode::ProjectDefault;
    UPROPERTY()
    EWorldPartitionDataLayersLogicOperator DataLayersLogicOperator = EWorldPartitionDataLayersLogicOperator::Or;
    UPROPERTY()
    uint8 bAllowShowingHLODsInEditor : 1 = true;
    UPROPERTY()
    uint8 bDisableContentBundles : 1 = false;
    UPROPERTY()
    uint8 bHasStandaloneHLOD : 1 = false;
    UPROPERTY()
    uint8 bExternalizeHLODAssets : 1 = false;
    UPROPERTY()
    uint8 bIsStandaloneHLODWorld : 1 = false;
    UPROPERTY()
    TObjectPtr<UHLODLayer> DefaultHLODLayer = nullptr;
    UPROPERTY()
    TObjectPtr<UWorldPartitionStreamingPolicy> StreamingPolicy = nullptr;
};

}
