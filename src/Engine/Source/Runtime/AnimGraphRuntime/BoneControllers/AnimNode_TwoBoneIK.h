#pragma once

#include "Engine/Source/Runtime/AnimGraphRuntime/BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Engine/Source/Runtime/AnimationCore/CommonAnimTypes.h"
#include "Engine/Source/Runtime/Engine/Animation/BoneSocketReference.h"
#include "Engine/Source/Runtime/Engine/AnimationRuntime.h"

#include "AnimNode_TwoBoneIK.generated.h"

namespace ue5parse::runtime {

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
