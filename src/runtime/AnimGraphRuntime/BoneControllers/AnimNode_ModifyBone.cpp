#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_ModifyBone.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

FTransform FAnimNode_ModifyBone::ModifyBoneTransform(FComponentSpacePoseContext& Output, const FCompactPoseBoneIndex BoneIndex) const {
    FTransform NewBoneTM = Output.Pose.GetComponentSpaceTransform(BoneIndex);
    const FTransform& ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
    if (ScaleMode != BMM_Ignore) {
        FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, BoneIndex, ScaleSpace);
        NewBoneTM.SetScale3D(ScaleMode == BMM_Additive ? NewBoneTM.GetScale3D() * Scale : Scale);
        FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, BoneIndex, ScaleSpace);
    }
    if (RotationMode != BMM_Ignore) {
        FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, BoneIndex, RotationSpace);
        const FQuat BoneQuat(Rotation);
        NewBoneTM.SetRotation(RotationMode == BMM_Additive ? BoneQuat * NewBoneTM.GetRotation() : BoneQuat);
        FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, BoneIndex, RotationSpace);
    }
    if (TranslationMode != BMM_Ignore) {
        FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, NewBoneTM, BoneIndex, TranslationSpace);
        if (TranslationMode == BMM_Additive) NewBoneTM.AddToTranslation(Translation);
        else NewBoneTM.SetTranslation(Translation);
        FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, NewBoneTM, BoneIndex, TranslationSpace);
    }
    return NewBoneTM;
}

void FAnimNode_ModifyBone::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    const FCompactPoseBoneIndex BoneIndex = BoneToModify.GetCompactPoseIndex(Output.Pose.GetPose().GetBoneContainer());
    OutBoneTransforms.Add(FBoneTransform(BoneIndex, ModifyBoneTransform(Output, BoneIndex)));
}

}
