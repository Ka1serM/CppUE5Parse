#pragma once

#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "ue5parse/runtime/AnimationCore/CommonAnimTypes.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/BoneSocketReference.h"

#include "AnimNode_TwoBoneIK.generated.h"

namespace ue5parse::runtime {

// Moves a bone and its two parents so that it reaches an effector, bending toward a joint target (UE's
// BoneControllers/AnimNode_TwoBoneIK.h).
USTRUCT()
struct FAnimNode_TwoBoneIK : public FAnimNode_SkeletalControlBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FBoneReference IKBone;
    UPROPERTY()
    double StartStretchRatio = 1.0;
    UPROPERTY()
    double MaxStretchScale = 1.2;
    UPROPERTY()
    FVector EffectorLocation = FVector::ZeroVector;
    UPROPERTY()
    FBoneSocketTarget EffectorTarget;
    UPROPERTY()
    FVector JointTargetLocation = FVector::ZeroVector;
    UPROPERTY()
    FBoneSocketTarget JointTarget;
    UPROPERTY()
    FAxis TwistAxis;
    UPROPERTY()
    EBoneControlSpace EffectorLocationSpace = BCS_ComponentSpace;
    UPROPERTY()
    EBoneControlSpace JointTargetLocationSpace = BCS_ComponentSpace;
    UPROPERTY()
    bool bAllowStretching = false;
    UPROPERTY()
    bool bTakeRotationFromEffectorSpace = false;
    UPROPERTY()
    bool bMaintainEffectorRelRot = false;
    UPROPERTY()
    bool bAllowTwist = true;

    void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;

    // The component space transform of a target: the offset in the target bone's space, or a location in a space
    // relative to it.
    static FTransform GetTargetTransform(const FTransform& InComponentTransform, FCSPose<FCompactPose>& MeshBases, const FBoneSocketTarget& InTarget,
                                         EBoneControlSpace Space, const FVector& InOffset);

protected:
    void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
    bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
    void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;

private:
    FCompactPoseBoneIndex CachedUpperLimbIndex = FCompactPoseBoneIndex(INDEX_NONE);
    FCompactPoseBoneIndex CachedLowerLimbIndex = FCompactPoseBoneIndex(INDEX_NONE);
};

}
