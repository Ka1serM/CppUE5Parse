#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_RotationMultiplier.h"

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

static FVector GetAxisVector(EBoneAxis Axis) {
    switch (Axis) {
        case BA_Y: return FVector(0.0, 1.0, 0.0);
        case BA_Z: return FVector(0.0, 0.0, 1.0);
        default: return FVector(1.0, 0.0, 0.0);
    }
}

// The rotation of a local bone transform relative to its reference rotation, after the axis of the bone is turned
// back onto the reference axis by the shortest arc: what remains is the twist around the axis.
static FQuat ExtractAngle(const FTransform& RefPoseTransform, const FTransform& LocalBoneTransform, EBoneAxis Axis) {
    const FVector RotationAxis = GetAxisVector(Axis);
    const FQuat ReferenceRotation = RefPoseTransform.GetRotation();
    const FVector LocalRotationVector = LocalBoneTransform.GetRotation().RotateVector(RotationAxis);
    const FVector ReferenceRotationVector = ReferenceRotation.RotateVector(RotationAxis);
    const FQuat BoneQuatAligned = FQuat::FindBetweenNormals(LocalRotationVector, ReferenceRotationVector) * LocalBoneTransform.GetRotation();
    return ReferenceRotation.Inverse() * BoneQuatAligned;
}

// The reference rotation of the target turned by the source's twist angle times the multiplier.
static FQuat MultiplyQuatBasedOnSourceIndex(const FTransform& RefPoseTransform, const FTransform& LocalBoneTransform, EBoneAxis Axis,
                                            float InMultiplier, const FQuat& ReferenceQuat) {
    FVector RotationAxis;
    double RotationAngle;
    ExtractAngle(RefPoseTransform, LocalBoneTransform, Axis).ToAxisAndAngle(RotationAxis, RotationAngle);
    // UE takes the angle as a float.
    float Angle = static_cast<float>(RotationAngle);
    if ((RotationAxis | GetAxisVector(Axis)) < 0.0) {
        RotationAxis = -RotationAxis;
        Angle = -Angle;
    }
    Angle = static_cast<float>(FMath::UnwindRadians(Angle));
    FQuat OutQuat = ReferenceQuat * FQuat(RotationAxis, double(Angle * InMultiplier));
    OutQuat.Normalize();
    return OutQuat;
}

void FAnimNode_RotationMultiplier::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    if (Multiplier == 0.f) return;
    const FCompactPose& Pose = Output.Pose.GetPose();
    const FCompactPoseBoneIndex TargetBoneIndex = TargetBone.GetCompactPoseIndex(Pose.GetBoneContainer());
    const FCompactPoseBoneIndex SourceBoneIndex = SourceBone.GetCompactPoseIndex(Pose.GetBoneContainer());
    FQuat NewQuat = MultiplyQuatBasedOnSourceIndex(Pose.GetRefPose(SourceBoneIndex), Output.Pose.GetLocalSpaceTransform(SourceBoneIndex),
                                                   RotationAxisToRefer.GetValue(), Multiplier, Pose.GetRefPose(TargetBoneIndex).GetRotation());
    FTransform NewLocalTransform = Output.Pose.GetLocalSpaceTransform(TargetBoneIndex);
    if (bIsAdditive) NewQuat = NewLocalTransform.GetRotation() * NewQuat;
    NewLocalTransform.SetRotation(NewQuat);
    const FCompactPoseBoneIndex ParentIndex = Pose.GetParentBoneIndex(TargetBoneIndex);
    if (ParentIndex != INDEX_NONE) NewLocalTransform *= Output.Pose.GetComponentSpaceTransform(ParentIndex);
    OutBoneTransforms.Add(FBoneTransform(TargetBoneIndex, NewLocalTransform));
}

bool FAnimNode_RotationMultiplier::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) {
    return TargetBone.IsValidToEvaluate(RequiredBones) && (TargetBone == SourceBone || SourceBone.IsValidToEvaluate(RequiredBones));
}

void FAnimNode_RotationMultiplier::InitializeBoneReferences(const FBoneContainer& RequiredBones) {
    SourceBone.Initialize(RequiredBones);
    TargetBone.Initialize(RequiredBones);
}

}
