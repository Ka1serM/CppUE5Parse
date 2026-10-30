#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_ObserveBone.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_ObserveBone::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
    const FCompactPoseBoneIndex BoneIndex = BoneToObserve.GetCompactPoseIndex(BoneContainer);
    FTransform BoneTM = Output.Pose.GetComponentSpaceTransform(BoneIndex);
    FAnimationRuntime::ConvertCSTransformToBoneSpace(Output.AnimInstanceProxy->GetComponentTransform(), Output.Pose, BoneTM, BoneIndex, DisplaySpace);
    if (bRelativeToRefPose) BoneTM = BoneTM.GetRelativeTransform(BoneContainer.GetRefPoseArray()[BoneToObserve.BoneIndex]);
    Translation = BoneTM.GetTranslation();
    Rotation = BoneTM.GetRotation().Rotator();
    Scale = BoneTM.GetScale3D();
}

bool FAnimNode_ObserveBone::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) {
    return BoneToObserve.IsValidToEvaluate(RequiredBones);
}

void FAnimNode_ObserveBone::InitializeBoneReferences(const FBoneContainer& RequiredBones) { BoneToObserve.Initialize(RequiredBones); }

}
