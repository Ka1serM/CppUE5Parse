#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_CopyBoneDelta.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"

namespace ue5parse::runtime {

void FAnimNode_CopyBoneDelta::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    if (!bCopyTranslation && !bCopyRotation && !bCopyScale) return;
    const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
    const FCompactPoseBoneIndex SourceBoneIndex = SourceBone.GetCompactPoseIndex(BoneContainer);
    const FCompactPoseBoneIndex TargetBoneIndex = TargetBone.GetCompactPoseIndex(BoneContainer);
    const FTransform& ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
    FTransform SourceTM = Output.Pose.GetComponentSpaceTransform(SourceBoneIndex);
    FTransform TargetTM = Output.Pose.GetComponentSpaceTransform(TargetBoneIndex);
    FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, SourceTM, SourceBoneIndex, BCS_ParentBoneSpace);
    FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, Output.Pose, TargetTM, TargetBoneIndex, BCS_ParentBoneSpace);
    SourceTM.SetToRelativeTransform(BoneContainer.GetRefPoseArray()[SourceBone.GetMeshPoseIndex(BoneContainer).GetInt()]);

    const bool bAccumulate = CopyMode == CopyBoneDeltaMode::Accumulate;
    if (bCopyTranslation) {
        const FVector Translation = SourceTM.GetTranslation() * double(TranslationMultiplier);
        if (bAccumulate) TargetTM.AddToTranslation(Translation);
        else TargetTM.SetTranslation(Translation);
    }
    if (bCopyRotation) {
        FVector Axis;
        double Angle;
        SourceTM.GetRotation().ToAxisAndAngle(Axis, Angle);
        // UE takes the angle as a float.
        const FQuat Rotation(Axis, double(static_cast<float>(Angle) * RotationMultiplier));
        TargetTM.SetRotation(bAccumulate ? Rotation * TargetTM.GetRotation() : Rotation);
    }
    if (bCopyScale) {
        const FVector Scale = SourceTM.GetScale3D() * double(ScaleMultiplier);
        TargetTM.SetScale3D(bAccumulate ? TargetTM.GetScale3D() * Scale : Scale);
    }
    FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, Output.Pose, TargetTM, TargetBoneIndex, BCS_ParentBoneSpace);
    OutBoneTransforms.Add(FBoneTransform(TargetBoneIndex, TargetTM));
}

bool FAnimNode_CopyBoneDelta::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) {
    return TargetBone.IsValidToEvaluate(RequiredBones) && (TargetBone == SourceBone || SourceBone.IsValidToEvaluate(RequiredBones));
}

void FAnimNode_CopyBoneDelta::InitializeBoneReferences(const FBoneContainer& RequiredBones) {
    SourceBone.Initialize(RequiredBones);
    TargetBone.Initialize(RequiredBones);
}

}
