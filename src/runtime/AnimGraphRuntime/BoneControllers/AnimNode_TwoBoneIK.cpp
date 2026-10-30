#include "ue5parse/runtime/AnimGraphRuntime/BoneControllers/AnimNode_TwoBoneIK.h"

#include "ue5parse/runtime/AnimationCore/TwoBoneIK.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

FTransform FAnimNode_TwoBoneIK::GetTargetTransform(const FTransform& InComponentTransform, FCSPose<FCompactPose>& MeshBases,
                                                   const FBoneSocketTarget& InTarget, EBoneControlSpace Space, const FVector& InOffset) {
    if (Space == BCS_BoneSpace) return InTarget.GetTargetTransform(InOffset, MeshBases, InComponentTransform);
    // A parent bone space is relative to the target's bone too (for a socket, the socket's bone).
    FTransform OutTransform;
    OutTransform.SetLocation(InOffset);
    FAnimationRuntime::ConvertBoneSpaceTransformToCS(InComponentTransform, MeshBases, OutTransform, InTarget.GetCompactPoseBoneIndex(), Space);
    return OutTransform;
}

// Removes the twist of a solved bone around an axis and puts back the twist of its local transform before the solve.
static void RemoveTwist(const FTransform& InParentTransform, FTransform& InOutTransform, const FTransform& OriginalLocalTransform,
                        const FVector& InAlignVector) {
    FQuat NewTwist, NewSwing;
    InOutTransform.GetRelativeTransform(InParentTransform).GetRotation().ToSwingTwist(InAlignVector, NewSwing, NewTwist);
    NewSwing.Normalize();
    FQuat OldTwist, OldSwing;
    OriginalLocalTransform.GetRotation().ToSwingTwist(InAlignVector, OldSwing, OldTwist);
    OldTwist.Normalize();
    InOutTransform.SetRotation(InParentTransform.GetRotation() * NewSwing * OldTwist);
    InOutTransform.NormalizeRotation();
}

void FAnimNode_TwoBoneIK::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) {
    const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();
    const FCompactPoseBoneIndex IKBoneCompactPoseIndex = IKBone.GetCompactPoseIndex(BoneContainer);
    const bool bInBoneSpace = EffectorLocationSpace == BCS_ParentBoneSpace || EffectorLocationSpace == BCS_BoneSpace;

    const FTransform EndBoneLocalTransform = Output.Pose.GetLocalSpaceTransform(IKBoneCompactPoseIndex);
    const FTransform LowerLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedLowerLimbIndex);
    const FTransform UpperLimbLocalTransform = Output.Pose.GetLocalSpaceTransform(CachedUpperLimbIndex);
    FTransform LowerLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex);
    FTransform UpperLimbCSTransform = Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex);
    FTransform EndBoneCSTransform = Output.Pose.GetComponentSpaceTransform(IKBoneCompactPoseIndex);

    const FTransform& ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
    const FTransform EffectorTransform = GetTargetTransform(ComponentTransform, Output.Pose, EffectorTarget, EffectorLocationSpace, EffectorLocation);
    const FTransform JointTargetTransform =
        GetTargetTransform(ComponentTransform, Output.Pose, JointTarget, JointTargetLocationSpace, JointTargetLocation);
    AnimationCore::SolveTwoBoneIK(UpperLimbCSTransform, LowerLimbCSTransform, EndBoneCSTransform, JointTargetTransform.GetTranslation(),
                                  EffectorTransform.GetTranslation(), bAllowStretching, StartStretchRatio, MaxStretchScale);

    if (!bAllowTwist) {
        const FCompactPoseBoneIndex UpperLimbParentIndex = BoneContainer.GetParentBoneIndex(CachedUpperLimbIndex);
        const FVector AlignDir = TwistAxis.GetTransformedAxis(FTransform::Identity);
        if (UpperLimbParentIndex != INDEX_NONE)
            RemoveTwist(Output.Pose.GetComponentSpaceTransform(UpperLimbParentIndex), UpperLimbCSTransform, UpperLimbLocalTransform, AlignDir);
        RemoveTwist(UpperLimbCSTransform, LowerLimbCSTransform, LowerLimbLocalTransform, AlignDir);
    }

    // The order matters: the upper limb, the lower limb, then the end bone.
    OutBoneTransforms.Add(FBoneTransform(CachedUpperLimbIndex, UpperLimbCSTransform));
    OutBoneTransforms.Add(FBoneTransform(CachedLowerLimbIndex, LowerLimbCSTransform));
    if (bInBoneSpace && bTakeRotationFromEffectorSpace) EndBoneCSTransform.SetRotation(EffectorTransform.GetRotation());
    else if (bMaintainEffectorRelRot) EndBoneCSTransform = EndBoneLocalTransform * LowerLimbCSTransform;
    OutBoneTransforms.Add(FBoneTransform(IKBoneCompactPoseIndex, EndBoneCSTransform));
}

bool FAnimNode_TwoBoneIK::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) {
    if (!IKBone.IsValidToEvaluate(RequiredBones)) return false;
    if (CachedUpperLimbIndex == INDEX_NONE || CachedLowerLimbIndex == INDEX_NONE) return false;
    const auto IsTargetSpace = [](EBoneControlSpace Space) { return Space == BCS_ParentBoneSpace || Space == BCS_BoneSpace; };
    if (IsTargetSpace(EffectorLocationSpace) && !EffectorTarget.IsValidToEvaluate(RequiredBones)) return false;
    return !IsTargetSpace(JointTargetLocationSpace) || JointTarget.IsValidToEvaluate(RequiredBones);
}

void FAnimNode_TwoBoneIK::InitializeBoneReferences(const FBoneContainer& RequiredBones) {
    IKBone.Initialize(RequiredBones);
    EffectorTarget.InitializeBoneReferences(RequiredBones);
    JointTarget.InitializeBoneReferences(RequiredBones);
    const FCompactPoseBoneIndex IKBoneCompactPoseIndex = IKBone.GetCompactPoseIndex(RequiredBones);
    CachedLowerLimbIndex = FCompactPoseBoneIndex(INDEX_NONE);
    CachedUpperLimbIndex = FCompactPoseBoneIndex(INDEX_NONE);
    if (IKBoneCompactPoseIndex == INDEX_NONE) return;
    CachedLowerLimbIndex = RequiredBones.GetParentBoneIndex(IKBoneCompactPoseIndex);
    if (CachedLowerLimbIndex != INDEX_NONE) CachedUpperLimbIndex = RequiredBones.GetParentBoneIndex(CachedLowerLimbIndex);
}

void FAnimNode_TwoBoneIK::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);
    EffectorTarget.Initialize(Context.AnimInstanceProxy);
    JointTarget.Initialize(Context.AnimInstanceProxy);
}

}
