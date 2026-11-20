#pragma once

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

#include "InstancedStaticMeshComponent.generated.h"

namespace ue5parse::runtime {

class UPhysicsSerializer;

USTRUCT()
struct FInstancedStaticMeshRandomSeed {
    GENERATED_BODY()
    UPROPERTY()
    int32 StartInstanceIndex = 0;
    UPROPERTY()
    int32 RandomSeed = 0;
};

USTRUCT()
struct FBoundsCacheElement {
    GENERATED_BODY()
    UPROPERTY()
    bool bIsValid = false;
    UPROPERTY()
    uint32 Hash = 0;
    UPROPERTY()
    FBoxSphereBounds Value;
};

USTRUCT()
struct FInstancedStaticMeshInstanceData {
    GENERATED_BODY()
    UPROPERTY()
    FMatrix Transform = FMatrix::Identity;
};

UCLASS()
class UInstancedStaticMeshComponent : public UStaticMeshComponent {
    GENERATED_BODY()
public:
    explicit UInstancedStaticMeshComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TArray<FInstancedStaticMeshInstanceData> PerInstanceSMData;
    UPROPERTY()
    int32 NumCustomDataFloats = 0;
    UPROPERTY()
    TArray<float> PerInstanceSMCustomData;
    UPROPERTY()
    int32 InstancingRandomSeed = 0;
    UPROPERTY()
    TArray<FInstancedStaticMeshRandomSeed> AdditionalRandomSeeds;
    UPROPERTY()
    float InstanceLODDistanceScale = 1.0f;
    UPROPERTY()
    int32 InstanceMinDrawDistance = 0;
    UPROPERTY()
    int32 InstanceStartCullDistance = 0;
    UPROPERTY()
    int32 InstanceEndCullDistance = 0;
    UPROPERTY()
    uint8 bUseGpuLodSelection : 1 = true;
    UPROPERTY()
    uint8 bInheritPerInstanceData : 1 = false;
    UPROPERTY()
    uint8 bDisableCollision : 1 = false;
    UPROPERTY()
    TArray<int32> InstanceReorderTable;
    UPROPERTY()
    TArray<int32> RemovedInstances;
    UPROPERTY()
    TObjectPtr<UPhysicsSerializer> PhysicsSerializer = nullptr;
    UPROPERTY()
    bool KeepInstanceBufferCPUAccess = true;
    UPROPERTY()
    FBoundsCacheElement CachedBounds[3];

    void Serialize(FArchive& Ar) override;
    UFUNCTION(BlueprintCallable)
    int32 GetInstanceCount() const { return PerInstanceSMData.Num(); }
    bool GetInstanceTransform(int32 InstanceIndex, FTransform& OutInstanceTransform) const;

private:
    bool InheritPerInstanceData();
};

}
