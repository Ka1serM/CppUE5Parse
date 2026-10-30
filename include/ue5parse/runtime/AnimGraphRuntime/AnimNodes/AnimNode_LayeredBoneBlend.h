#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimData/BoneMaskFilter.h"
#include "ue5parse/runtime/Engine/Animation/AnimNodeBase.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"

#include "AnimNode_LayeredBoneBlend.generated.h"

namespace ue5parse::runtime {

class UBlendProfile;
class USkeleton;

enum class ELayeredBoneBlendMode : uint8 { BranchFilter, BlendMask };

// Blends layers over a base pose per bone: each layer owns the skeleton branches of its filter (or the bones of its
// blend mask) with a weight (UE's AnimNodes/AnimNode_LayeredBoneBlend.h).
USTRUCT()
struct FAnimNode_LayeredBoneBlend : public FAnimNode_Base {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPoseLink BasePose;
    UPROPERTY()
    TArray<FPoseLink> BlendPoses;
    UPROPERTY()
    ELayeredBoneBlendMode BlendMode = ELayeredBoneBlendMode::BranchFilter;
    UPROPERTY()
    TArray<UBlendProfile*> BlendMasks;
    UPROPERTY()
    TArray<FInputBlendPose> LayerSetup;
    UPROPERTY()
    TArray<float> BlendWeights;
    UPROPERTY()
    bool bMeshSpaceRotationBlend = false;
    UPROPERTY()
    bool bMeshSpaceScaleBlend = false;
    UPROPERTY()
    TEnumAsByte<ECurveBlendOption::Type> CurveBlendOption = ECurveBlendOption::Override;
    UPROPERTY()
    bool bBlendRootMotionBasedOnRootBone = true;
    bool bHasRelevantPoses = false;
    UPROPERTY()
    int32 LODThreshold = INDEX_NONE;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
    void CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) override;
    void Update_AnyThread(const FAnimationUpdateContext& Context) override;
    void Evaluate_AnyThread(FPoseContext& Output) override;

private:
    void RebuildPerBoneBlendWeights(const USkeleton* InSkeleton);
    bool ArePerBoneBlendWeightsValid(const USkeleton* InSkeleton) const;
    void UpdateCachedBoneData(const FBoneContainer& RequiredBones, const USkeleton* Skeleton);

protected:
    // By skeleton bone (cooked with the guids of the skeleton it was built for), then by compact bone: the layer
    // weights as authored, and times the layers' current weights.
    UPROPERTY()
    TArray<FPerBoneBlendWeight> PerBoneBlendWeights;
    UPROPERTY()
    FGuid SkeletonGuid;
    UPROPERTY()
    FGuid VirtualBoneGuid;
    TArray<FPerBoneBlendWeight> DesiredBoneBlendWeights;
    TArray<FPerBoneBlendWeight> CurrentBoneBlendWeights;
    uint16 RequiredBonesSerialNumber = 0;
};

}
