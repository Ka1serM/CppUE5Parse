#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_BlendBoneByChannel.h"

#include <algorithm>

#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"

namespace ue5parse::runtime {

namespace {

void BlendChannels(const FBlendBoneByChannelEntry& Entry, FTransform& TargetTransform, const FTransform& SourceTransform, float Alpha) {
    FTransform BlendedTransform;
    BlendedTransform.Blend(TargetTransform, SourceTransform, Alpha);
    if (Entry.bBlendTranslation) TargetTransform.SetTranslation(BlendedTransform.GetTranslation());
    if (Entry.bBlendRotation) TargetTransform.SetRotation(BlendedTransform.GetRotation());
    if (Entry.bBlendScale) TargetTransform.SetScale3D(BlendedTransform.GetScale3D());
}

}

void FAnimNode_BlendBoneByChannel::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    A.Initialize(Context);
    B.Initialize(Context);
}

void FAnimNode_BlendBoneByChannel::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    A.CacheBones(Context);
    B.CacheBones(Context);
    ValidBoneEntries.Reset();
    const FBoneContainer& BoneContainer = Context.AnimInstanceProxy->GetRequiredBones();
    for (FBlendBoneByChannelEntry& Entry : BoneDefinitions) {
        Entry.SourceBone.Initialize(BoneContainer);
        Entry.TargetBone.Initialize(BoneContainer);
        if (Entry.SourceBone.IsValidToEvaluate(BoneContainer) && Entry.TargetBone.IsValidToEvaluate(BoneContainer) &&
            (Entry.bBlendTranslation || Entry.bBlendRotation || Entry.bBlendScale))
            ValidBoneEntries.Add(Entry);
    }
}

void FAnimNode_BlendBoneByChannel::Update_AnyThread(const FAnimationUpdateContext& Context) {
    GetEvaluateGraphExposedInputs().Execute(Context);
    InternalBlendAlpha = AlphaScaleBias.ApplyTo(Alpha);
    bBIsRelevant = FAnimWeight::IsRelevant(InternalBlendAlpha) && ValidBoneEntries.Num() > 0;
    A.Update(Context);
    if (bBIsRelevant) B.Update(Context.FractionalWeight(InternalBlendAlpha));
}

void FAnimNode_BlendBoneByChannel::Evaluate_AnyThread(FPoseContext& Output) {
    A.Evaluate(Output);
    if (!bBIsRelevant) return;
    const FBoneContainer& BoneContainer = Output.AnimInstanceProxy->GetRequiredBones();
    FPoseContext PoseB(Output);
    B.Evaluate(PoseB);
    if (TransformsSpace == BCS_BoneSpace) {
        for (const FBlendBoneByChannelEntry& Entry : ValidBoneEntries)
            BlendChannels(Entry, Output.Pose[Entry.TargetBone.GetCompactPoseIndex(BoneContainer)], PoseB.Pose[Entry.SourceBone.GetCompactPoseIndex(BoneContainer)],
                          InternalBlendAlpha);
        return;
    }
    FCSPose<FCompactPose> TargetPoseCmpntSpace;
    TargetPoseCmpntSpace.InitPose(Output.Pose);
    FCSPose<FCompactPose> SourcePoseCmpntSpace;
    SourcePoseCmpntSpace.InitPose(PoseB.Pose);
    TArray<FBoneTransform> QueuedModifiedBoneTransforms;
    const FTransform& ComponentTransform = Output.AnimInstanceProxy->GetComponentTransform();
    for (const FBlendBoneByChannelEntry& Entry : ValidBoneEntries) {
        const FCompactPoseBoneIndex SourceBoneIndex = Entry.SourceBone.GetCompactPoseIndex(BoneContainer);
        const FCompactPoseBoneIndex TargetBoneIndex = Entry.TargetBone.GetCompactPoseIndex(BoneContainer);
        FTransform SourceTransform = SourcePoseCmpntSpace.GetComponentSpaceTransform(SourceBoneIndex);
        FTransform TargetTransform = TargetPoseCmpntSpace.GetComponentSpaceTransform(TargetBoneIndex);
        FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, SourcePoseCmpntSpace, SourceTransform, SourceBoneIndex, TransformsSpace);
        FAnimationRuntime::ConvertCSTransformToBoneSpace(ComponentTransform, TargetPoseCmpntSpace, TargetTransform, TargetBoneIndex, TransformsSpace);
        BlendChannels(Entry, TargetTransform, SourceTransform, InternalBlendAlpha);
        FAnimationRuntime::ConvertBoneSpaceTransformToCS(ComponentTransform, TargetPoseCmpntSpace, TargetTransform, TargetBoneIndex, TransformsSpace);
        QueuedModifiedBoneTransforms.Add(FBoneTransform(TargetBoneIndex, TargetTransform));
    }
    if (QueuedModifiedBoneTransforms.Num() == 0) return;
    std::stable_sort(QueuedModifiedBoneTransforms.begin(), QueuedModifiedBoneTransforms.end(),
                     [](const FBoneTransform& X, const FBoneTransform& Y) { return X.BoneIndex.GetInt() < Y.BoneIndex.GetInt(); });
    TargetPoseCmpntSpace.SafeSetCSBoneTransforms(QueuedModifiedBoneTransforms);
    FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(std::move(TargetPoseCmpntSpace), Output.Pose);
}

}
