#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_CopyBone.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

void FAnimNode_CopyBone::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    if (!bCopyTranslation && !bCopyRotation && !bCopyScale) return;
    const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
    const FCompactPoseBoneIndex SourceBoneIndex = SourceBone.GetCompactPoseIndex(BoneContainer);
    const FCompactPoseBoneIndex TargetBoneIndex = TargetBone.GetCompactPoseIndex(BoneContainer);
    const FTransform& ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
    FTransform SourceBoneTM = Output.Pose.GetComponentSpaceTransform(SourceBoneIndex);
    FTransform CurrentBoneTM = Output.Pose.GetComponentSpaceTransform(TargetBoneIndex);
    if (ControlSpace != BCS_ComponentSpace) {
        FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, SourceBoneTM, SourceBoneIndex, ControlSpace);
        FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, CurrentBoneTM, TargetBoneIndex, ControlSpace);
    }
    if (bCopyTranslation) CurrentBoneTM.SetTranslation(SourceBoneTM.GetTranslation());
    if (bCopyRotation) CurrentBoneTM.SetRotation(SourceBoneTM.GetRotation());
    if (bCopyScale) CurrentBoneTM.SetScale3D(SourceBoneTM.GetScale3D());
    if (ControlSpace != BCS_ComponentSpace)
        FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, CurrentBoneTM, TargetBoneIndex, ControlSpace);
    OutBoneTransforms.Add(FBoneTransform(TargetBoneIndex, CurrentBoneTM));
}

bool FAnimNode_CopyBone::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) {
    return TargetBone.IsValidToEvaluate(RequiredBones) && (TargetBone == SourceBone || SourceBone.IsValidToEvaluate(RequiredBones));
}

void FAnimNode_CopyBone::InitializeBoneReferences(const FBoneContainer& RequiredBones) {
    SourceBone.Initialize(RequiredBones);
    TargetBone.Initialize(RequiredBones);
}

}
