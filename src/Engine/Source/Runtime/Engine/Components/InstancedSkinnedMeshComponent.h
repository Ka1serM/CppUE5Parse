#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Core/Math/Matrix.h"
#include "Engine/Source/Runtime/Core/Math/Transform.h"
#include "Engine/Source/Runtime/Engine/Components/SkinnedMeshComponent.h"

#include "InstancedSkinnedMeshComponent.generated.h"

namespace ue5parse::runtime {

class UBodySetup;

USTRUCT()
struct FSkinnedMeshInstanceData {
    GENERATED_BODY()

    UPROPERTY()
    FTransform3f Transform;
    UPROPERTY()
    uint32 AnimationIndex = 0;

    friend FArchive& operator<<(FArchive& Ar, FSkinnedMeshInstanceData& InstanceData) { return Ar << InstanceData.Transform << InstanceData.AnimationIndex; }
};

struct FSkinnedMeshInstanceData_Deprecated {
    FMatrix44f Transform;
    uint32 BankIndex = 0;
};

struct FCompressedSpatialHashItem {
    FInt64Vector Coord;
    int32 Level = 0;
    int32 NumInstances = 0;
};

struct FBoneAttachmentSocketReference {
    TObjectPtr<UObject> Parent = nullptr;
    FName SocketName;
    int32 SavedIndex = 0;
};

UCLASS()
class UInstancedSkinnedMeshComponent : public USkinnedMeshComponent {
    GENERATED_BODY()
public:
    explicit UInstancedSkinnedMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    TObjectPtr<UObject> TransformProvider = nullptr;
    UPROPERTY()
    TArray<FSkinnedMeshInstanceData> InstanceData;
    UPROPERTY()
    int32 NumCustomDataFloats = 0;
    UPROPERTY()
    TArray<float> InstanceCustomData;
    UPROPERTY()
    TArray<FTransform3f> PerInstancePrevTransform;
    UPROPERTY()
    float AnimationMinScreenSize = 0.0f;
    UPROPERTY()
    int32 InstanceMinDrawDistance = 0;
    UPROPERTY()
    int32 InstanceStartCullDistance = 0;
    UPROPERTY()
    int32 InstanceEndCullDistance = 0;
    UPROPERTY()
    bool bInheritPerInstanceData = false;
    UPROPERTY()
    FBox PrimitiveBoundsOverride;
    UPROPERTY()
    bool bIsInstanceDataGPUOnly = false;
    UPROPERTY()
    bool bHasPreviousTransforms = false;
    UPROPERTY()
    bool bForceAnimateSockets = false;
    UPROPERTY()
    bool bUseGpuLodSelection = true;
    UPROPERTY()
    int32 NumInstancesGPUOnly = 0;
    UPROPERTY()
    bool bDisableCollision = false;
    UPROPERTY()
    TObjectPtr<UBodySetup> MergedBodySetup = nullptr;

    TArray<int32> IndexToIdMap;
    int32 NumInstances = 0;
    int32 MaxInstanceId = 0;
    TArray<FBoneAttachmentSocketReference> BoneAttachmentSockets;
    TArray<uint32> BoneAttachmentBindings;
    TArray<FCompressedSpatialHashItem> Hashes;
    TArray<int32> ProxyIndexToComponentIndexRemap;
};

}
