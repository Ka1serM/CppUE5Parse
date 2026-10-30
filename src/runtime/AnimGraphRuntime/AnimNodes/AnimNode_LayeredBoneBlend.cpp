#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_LayeredBoneBlend.h"

#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/Animation/Skeleton.h"

// The skeleton's curve metadata (the bones a curve links to) is not loaded, so every curve blends by its layer's
// highest bone weight; UE gives a linked curve to the layer of its bone only.

namespace ue5parse::runtime {

void FAnimNode_LayeredBoneBlend::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    BasePose.Initialize(Context);
    for (FPoseLink& BlendPose : BlendPoses) BlendPose.Initialize(Context);
}

void FAnimNode_LayeredBoneBlend::RebuildPerBoneBlendWeights(const USkeleton* InSkeleton) {
    if (InSkeleton == nullptr) return;
    if (BlendMode == ELayeredBoneBlendMode::BranchFilter) FAnimationRuntime::CreateMaskWeights(PerBoneBlendWeights, LayerSetup, InSkeleton);
    else FAnimationRuntime::CreateMaskWeights(PerBoneBlendWeights, BlendMasks, InSkeleton);
    SkeletonGuid = InSkeleton->GetGuid();
    VirtualBoneGuid = InSkeleton->GetVirtualBoneGuid();
}

bool FAnimNode_LayeredBoneBlend::ArePerBoneBlendWeightsValid(const USkeleton* InSkeleton) const {
    return InSkeleton != nullptr && InSkeleton->GetGuid() == SkeletonGuid && InSkeleton->GetVirtualBoneGuid() == VirtualBoneGuid;
}

void FAnimNode_LayeredBoneBlend::UpdateCachedBoneData(const FBoneContainer& RequiredBones, const USkeleton* Skeleton) {
    if (RequiredBones.GetSerialNumber() == RequiredBonesSerialNumber) return;
    if (!ArePerBoneBlendWeightsValid(Skeleton)) RebuildPerBoneBlendWeights(Skeleton);
    const int32 NumRequiredBones = RequiredBones.GetCompactPoseNumBones();
    DesiredBoneBlendWeights.Reset();
    DesiredBoneBlendWeights.SetNum(NumRequiredBones);
    for (int32 RequiredBoneIndex = 0; RequiredBoneIndex < NumRequiredBones; RequiredBoneIndex++) {
        const int32 SkeletonBoneIndex = RequiredBones.GetSkeletonIndex(FCompactPoseBoneIndex(RequiredBoneIndex));
        if (SkeletonBoneIndex != INDEX_NONE) DesiredBoneBlendWeights[RequiredBoneIndex] = PerBoneBlendWeights[SkeletonBoneIndex];
    }
    CurrentBoneBlendWeights.Reset();
    CurrentBoneBlendWeights.SetNum(NumRequiredBones);
    FAnimationRuntime::UpdateDesiredBoneWeight(DesiredBoneBlendWeights, CurrentBoneBlendWeights, BlendWeights);
    RequiredBonesSerialNumber = RequiredBones.GetSerialNumber();
}

void FAnimNode_LayeredBoneBlend::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    BasePose.CacheBones(Context);
    for (FPoseLink& BlendPose : BlendPoses) BlendPose.CacheBones(Context);
    UpdateCachedBoneData(Context.AnimInstanceProxy->GetRequiredBones(), Context.AnimInstanceProxy->GetSkeleton());
}

void FAnimNode_LayeredBoneBlend::Update_AnyThread(const FAnimationUpdateContext& Context) {
    bHasRelevantPoses = false;
    int32 RootMotionBlendPose = -1;
    float RootMotionWeight = 0.f;
    const float RootMotionClearWeight = bBlendRootMotionBasedOnRootBone ? 0.f : 1.f;
    GetEvaluateGraphExposedInputs().Execute(Context);
    for (int32 ChildIndex = 0; ChildIndex < BlendPoses.Num(); ++ChildIndex) {
        const float ChildWeight = BlendWeights[ChildIndex];
        if (!FAnimWeight::IsRelevant(ChildWeight)) continue;
        if (!bHasRelevantPoses) {
            UpdateCachedBoneData(Context.AnimInstanceProxy->GetRequiredBones(), Context.AnimInstanceProxy->GetSkeleton());
            FAnimationRuntime::UpdateDesiredBoneWeight(DesiredBoneBlendWeights, CurrentBoneBlendWeights, BlendWeights);
            bHasRelevantPoses = true;
            if (bBlendRootMotionBasedOnRootBone && !CurrentBoneBlendWeights.IsEmpty()) {
                const float NewRootMotionWeight = CurrentBoneBlendWeights[0].BlendWeight;
                if (NewRootMotionWeight > ZERO_ANIMWEIGHT_THRESH) {
                    RootMotionWeight = NewRootMotionWeight;
                    RootMotionBlendPose = CurrentBoneBlendWeights[0].SourceIndex;
                }
            }
        }
        const float ThisPoseRootMotionWeight = ChildIndex == RootMotionBlendPose ? RootMotionWeight : RootMotionClearWeight;
        BlendPoses[ChildIndex].Update(Context.FractionalWeightAndRootMotion(ChildWeight, ThisPoseRootMotionWeight));
    }
    const float BaseRootMotionWeight = 1.f - RootMotionWeight;
    if (BaseRootMotionWeight < ZERO_ANIMWEIGHT_THRESH) BasePose.Update(Context.FractionalWeightAndRootMotion(1.f, BaseRootMotionWeight));
    else BasePose.Update(Context);
}

void FAnimNode_LayeredBoneBlend::Evaluate_AnyThread(FPoseContext& Output) {
    const int32 NumPoses = BlendPoses.Num();
    if (NumPoses == 0 || !bHasRelevantPoses) {
        BasePose.Evaluate(Output);
        return;
    }
    FPoseContext BasePoseContext(Output);
    BasePose.Evaluate(BasePoseContext);
    TArray<FCompactPose> TargetBlendPoses;
    TargetBlendPoses.SetNum(NumPoses);
    TArray<FBlendedCurve> TargetBlendCurves;
    TargetBlendCurves.SetNum(NumPoses);
    for (int32 ChildIndex = 0; ChildIndex < NumPoses; ++ChildIndex) {
        if (FAnimWeight::IsRelevant(BlendWeights[ChildIndex])) {
            FPoseContext CurrentPoseContext(Output);
            BlendPoses[ChildIndex].Evaluate(CurrentPoseContext);
            TargetBlendPoses[ChildIndex].MoveBonesFrom(CurrentPoseContext.Pose);
            TargetBlendCurves[ChildIndex] = std::move(CurrentPoseContext.Curve);
            continue;
        }
        TargetBlendPoses[ChildIndex].SetBoneContainer(&BasePoseContext.Pose.GetBoneContainer());
        TargetBlendPoses[ChildIndex].ResetToRefPose();
    }
    FAnimationRuntime::EBlendPosesPerBoneFilterFlags BlendFlags = FAnimationRuntime::EBlendPosesPerBoneFilterFlags::None;
    if (bMeshSpaceRotationBlend) BlendFlags |= FAnimationRuntime::EBlendPosesPerBoneFilterFlags::MeshSpaceRotation;
    if (bMeshSpaceScaleBlend) BlendFlags |= FAnimationRuntime::EBlendPosesPerBoneFilterFlags::MeshSpaceScale;
    FAnimationPoseData AnimationPoseData = Output.GetAnimationPoseData();
    FAnimationRuntime::BlendPosesPerBoneFilter(BasePoseContext.Pose, TargetBlendPoses, BasePoseContext.Curve, TargetBlendCurves, AnimationPoseData,
                                               CurrentBoneBlendWeights, BlendFlags, CurveBlendOption);
}

}
