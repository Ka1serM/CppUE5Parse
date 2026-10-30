#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_Constraint.h"

#include "ue5parse/runtime/Engine/AnimationRuntime.h"

namespace ue5parse::runtime {

void FAnimNode_Constraint::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
    for (int32 ConstraintIndex = 0; ConstraintIndex < ConstraintSetup.Num(); ++ConstraintIndex) {
        const FConstraint& Constraint = ConstraintSetup[ConstraintIndex];
        const float Weight = ConstraintWeights[ConstraintIndex];
        if (Weight <= ZERO_ANIMWEIGHT_THRESH || !Constraint.IsValidToEvaluate(BoneContainer) || Constraint.ConstraintDataIndex == INDEX_NONE) continue;
        ConstraintData[Constraint.ConstraintDataIndex].Weight = Weight;
        ConstraintData[Constraint.ConstraintDataIndex].CurrentTransform = Output.Pose.GetComponentSpaceTransform(Constraint.TargetBone.CachedCompactPoseIndex);
    }
    if (ConstraintData.Num() == 0) return;
    const FTransform SourceTransform = Output.Pose.GetComponentSpaceTransform(BoneToModify.CachedCompactPoseIndex);
    const FTransform ConstrainedTransform = AnimationCore::SolveConstraints(SourceTransform, FTransform::Identity, ConstraintData);
    OutBoneTransforms.Add(FBoneTransform(BoneToModify.CachedCompactPoseIndex, ConstrainedTransform));
}

bool FAnimNode_Constraint::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) {
    bool bHaveValidConstraint = false;
    for (int32 ConstraintIndex = 0; ConstraintIndex < ConstraintSetup.Num(); ++ConstraintIndex)
        if (ConstraintWeights[ConstraintIndex] > ZERO_ANIMWEIGHT_THRESH) bHaveValidConstraint |= ConstraintSetup[ConstraintIndex].IsValidToEvaluate(RequiredBones);
    return bHaveValidConstraint && BoneToModify.IsValidToEvaluate(RequiredBones);
}

void FAnimNode_Constraint::InitializeBoneReferences(const FBoneContainer& RequiredBones) {
    BoneToModify.Initialize(RequiredBones);
    ConstraintData.Reset(ConstraintSetup.Num());
    if (!BoneToModify.IsValidToEvaluate(RequiredBones)) return;
    const FTransform SourceTransform = FAnimationRuntime::GetComponentSpaceRefPose(BoneToModify.CachedCompactPoseIndex, RequiredBones);
    for (FConstraint& Constraint : ConstraintSetup) {
        Constraint.Initialize(RequiredBones);
        if (!Constraint.TargetBone.IsValidToEvaluate(RequiredBones)) {
            Constraint.ConstraintDataIndex = INDEX_NONE;
            continue;
        }
        FConstraintData NewConstraintData(FTransformConstraintDescription(Constraint.TransformType), Constraint.TargetBone.BoneName, 0.f,
                                          Constraint.OffsetOption != EConstraintOffsetOption::None);
        NewConstraintData.Constraint.ConstraintDescription->AxesFilterOption = Constraint.PerAxis;
        const FTransform TargetTransform = NewConstraintData.bMaintainOffset
                                               ? FAnimationRuntime::GetComponentSpaceRefPose(Constraint.TargetBone.CachedCompactPoseIndex, RequiredBones)
                                               : FTransform::Identity;
        NewConstraintData.SaveInverseOffset(SourceTransform, TargetTransform, FTransform::Identity);
        Constraint.ConstraintDataIndex = ConstraintData.Add(NewConstraintData);
    }
}

}
