#include "ue5parse/runtime/AnimGraphRuntime/AnimNodes/AnimNode_PoseDriver.h"

#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/Engine/Animation/AnimInstanceProxy.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"

namespace ue5parse::runtime {

void FAnimNode_PoseDriver::Initialize_AnyThread(const FAnimationInitializeContext& Context) {
    FAnimNode_PoseHandler::Initialize_AnyThread(Context);
    SourcePose.Initialize(Context);
    if (RBFParams.SolverType == ERBFSolverType::Interpolative)
        ReportLoadProblem("anim.rbf_interpolative", FString("a pose driver uses the interpolative RBF solver, which gives no weights"));
}

void FAnimNode_PoseDriver::RebuildPoseList(const FBoneContainer& InBoneContainer, const UPoseAsset* InPoseAsset) {
    PoseExtractContext.PoseCurves.Reset();
    if (InPoseAsset->GetSkeleton() == nullptr) return;
    const TArray<FName>& PoseNames = InPoseAsset->GetPoseFNames();
    for (FPoseDriverTarget& PoseTarget : PoseTargets) {
        const int32 PoseIndex = InPoseAsset->GetPoseIndexByName(PoseTarget.DrivenName);
        PoseTarget.PoseCurveIndex =
            PoseIndex == INDEX_NONE ? INDEX_NONE : PoseExtractContext.PoseCurves.Add(FPoseCurve(PoseIndex, PoseNames[PoseIndex], 0.f));
    }
}

void FAnimNode_PoseDriver::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context) {
    FAnimNode_PoseHandler::CacheBones_AnyThread(Context);
    SourcePose.CacheBones(Context);
    const FBoneContainer& BoneContainer = Context.AnimInstanceProxy->GetRequiredBones();
    for (FBoneReference& SourceBone : SourceBones) SourceBone.Initialize(BoneContainer);
    for (FBoneReference& OnlyDriveBone : OnlyDriveBones) OnlyDriveBone.Initialize(BoneContainer);
    EvalSpaceBone.Initialize(BoneContainer);

    // The source bones keep their input.
    for (const FBoneReference& SourceBone : SourceBones) {
        const FCompactPoseBoneIndex SourceIndex = SourceBone.GetCompactPoseIndex(BoneContainer);
        if (BoneBlendWeights.IsValidIndex(SourceIndex.GetInt())) BoneBlendWeights[SourceIndex.GetInt()] = 0.f;
    }
    bHasOnlyDriveBones = OnlyDriveBones.IndexOfByPredicate([](const FBoneReference& Bone) { return !Bone.BoneName.isNone(); }) != INDEX_NONE;
    BonesToFilter.Reset();
    if (bHasOnlyDriveBones && CurrentPoseAsset != nullptr) {
        for (const FName& TrackName : CurrentPoseAsset->GetTrackNames()) {
            if (IsBoneDriven(TrackName)) continue;
            const FCompactPoseBoneIndex CompactBoneIndex =
                BoneContainer.MakeCompactPoseIndex(FMeshPoseBoneIndex(BoneContainer.GetPoseBoneIndexForBoneName(TrackName)));
            if (CompactBoneIndex == INDEX_NONE) continue;
            BoneBlendWeights[CompactBoneIndex.GetInt()] = 0.f;
            BonesToFilter.Add(CompactBoneIndex);
        }
    }
    PoseExtractContext.BonesRequired.SetNum(BoneBlendWeights.Num());
    for (int32 BoneIndex = 0; BoneIndex < BoneBlendWeights.Num(); BoneIndex++)
        PoseExtractContext.BonesRequired[BoneIndex] = BoneBlendWeights[BoneIndex] > UE_SMALL_NUMBER;
}

void FAnimNode_PoseDriver::UpdateAssetPlayer(const FAnimationUpdateContext& Context) {
    FAnimNode_PoseHandler::UpdateAssetPlayer(Context);
    SourcePose.Update(Context);
}

bool FAnimNode_PoseDriver::IsBoneDriven(FName BoneName) const {
    return OnlyDriveBones.IndexOfByPredicate([BoneName](const FBoneReference& Bone) { return Bone.BoneName == BoneName; }) != INDEX_NONE;
}

void FAnimNode_PoseDriver::GetRBFTargets(TArray<FRBFTarget>& OutTargets, const FBoneContainer& BoneContainer) const {
    OutTargets.Reset();
    OutTargets.SetNum(PoseTargets.Num());
    for (int32 TargetIndex = 0; TargetIndex < PoseTargets.Num(); TargetIndex++) {
        FRBFTarget& RBFTarget = OutTargets[TargetIndex];
        const FPoseDriverTarget& PoseTarget = PoseTargets[TargetIndex];
        // A target without a transform for a source bone has zeros for it.
        for (int32 SourceIndex = 0; SourceIndex < SourceBones.Num(); SourceIndex++) {
            if (!PoseTarget.BoneTransforms.IsValidIndex(SourceIndex)) {
                RBFTarget.AddFromVector(FVector::ZeroVector);
                continue;
            }
            const FPoseDriverTransform& BoneTransform = PoseTarget.BoneTransforms[SourceIndex];
            FTransform RefBoneTransform = FTransform::Identity;
            const FBoneReference& SourceBone = SourceBones[SourceIndex];
            if (bEvalFromRefPose && SourceBone.HasValidSetup() && SourceBone.CachedCompactPoseIndex < BoneContainer.GetCompactPoseNumBones())
                RefBoneTransform = BoneContainer.GetRefPoseTransform(SourceBone.CachedCompactPoseIndex);
            if (DriveSource == EPoseDriverSource::Translation) {
                RBFTarget.AddFromVector(bEvalFromRefPose ? RefBoneTransform.Inverse().TransformPosition(BoneTransform.TargetTranslation)
                                                         : BoneTransform.TargetTranslation);
            } else {
                RBFTarget.AddFromRotator(bEvalFromRefPose ? RefBoneTransform.Inverse().TransformRotation(BoneTransform.TargetRotation.Quaternion()).Rotator()
                                                          : BoneTransform.TargetRotation);
            }
        }
        RBFTarget.ScaleFactor = PoseTarget.TargetScale;
        RBFTarget.bApplyCustomCurve = PoseTarget.bApplyCustomCurve;
        RBFTarget.CustomCurve = PoseTarget.CustomCurve;
        RBFTarget.DistanceMethod = PoseTarget.DistanceMethod;
        RBFTarget.FunctionType = PoseTarget.FunctionType;
    }
}

void FAnimNode_PoseDriver::Evaluate_AnyThread(FPoseContext& Output) {
    FPoseContext SourceData(Output);
    SourcePose.Evaluate(SourceData);
    const FBoneContainer& BoneContainer = SourceData.Pose.GetBoneContainer();

    // The input: each source bone's transform in the evaluation space bone's space, else in its local space, which
    // bEvalFromRefPose makes relative to the reference pose.
    RBFInput.Values.Reset();
    bool bFoundAnyBone = false;
    for (const FBoneReference& SourceBone : SourceBones) {
        FTransform SourceBoneTM = FTransform::Identity;
        const FCompactPoseBoneIndex SourceIndex = SourceBone.GetCompactPoseIndex(BoneContainer);
        if (SourceIndex != INDEX_NONE) {
            if (EvalSpaceBone.IsValidToEvaluate(BoneContainer)) {
                FCSPose<FCompactPose> CSPose;
                CSPose.InitPose(SourceData.Pose);
                const FTransform EvalSpaceTM = CSPose.GetComponentSpaceTransform(EvalSpaceBone.GetCompactPoseIndex(BoneContainer));
                SourceBoneTM = CSPose.GetComponentSpaceTransform(SourceIndex).GetRelativeTransform(EvalSpaceTM);
            } else if (bEvalFromRefPose && SourceIndex.GetInt() < BoneContainer.GetCompactPoseNumBones()) {
                SourceBoneTM = SourceData.Pose[SourceIndex].GetRelativeTransform(BoneContainer.GetRefPoseTransform(SourceIndex));
            } else {
                SourceBoneTM = SourceData.Pose[SourceIndex];
            }
            bFoundAnyBone = true;
        }
        if (DriveSource == EPoseDriverSource::Translation) RBFInput.AddFromVector(SourceBoneTM.GetTranslation());
        else RBFInput.AddFromRotator(SourceBoneTM.Rotator());
    }
    if (!bFoundAnyBone) {
        Output = SourceData;
        return;
    }

    RBFParams.TargetDimensions = SourceBones.Num() * 3;
    OutputWeights.Reset();
    GetRBFTargets(RBFTargets, BoneContainer);
    FRBFSolver::Solve(RBFParams, RBFTargets, RBFInput, OutputWeights);
    if (OutputWeights.Num() == 0) {
        Output = SourceData;
        return;
    }

    if (DriveOutput == EPoseDriverOutput::DriveCurves) {
        Output = SourceData;
        FBlendedCurve DrivenCurves;
        for (const FRBFOutputWeight& Weight : OutputWeights) {
            const FName DrivenName = PoseTargets[Weight.TargetIndex].DrivenName;
            if (!DrivenName.isNone()) DrivenCurves.Set(DrivenName, Weight.TargetWeight);
        }
        Output.Curve.Combine(DrivenCurves);
        return;
    }

    if (CurrentPoseAsset == nullptr || CurrentPoseAsset->GetSkeleton() == nullptr) {
        Output = SourceData;
        return;
    }
    for (FPoseCurve& PoseCurve : PoseExtractContext.PoseCurves) PoseCurve.Value = 0.f;
    for (const FRBFOutputWeight& Weight : OutputWeights) {
        const int32 PoseCurveIndex = PoseTargets[Weight.TargetIndex].PoseCurveIndex;
        if (PoseCurveIndex != INDEX_NONE) PoseExtractContext.PoseCurves[PoseCurveIndex].Value = Weight.TargetWeight;
    }
    FPoseContext CurrentPose(Output);
    FAnimationPoseData CurrentAnimationPoseData = CurrentPose.GetAnimationPoseData();
    if (!CurrentPoseAsset->GetAnimationPose(CurrentAnimationPoseData, PoseExtractContext)) {
        Output = SourceData;
        return;
    }
    if (CurrentPoseAsset->IsValidAdditive()) {
        // The source bones and the filtered bones get the additive identity, with a zero scale, so they keep the input.
        const FTransform AdditiveIdentity(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector);
        for (const FBoneReference& SourceBone : SourceBones) {
            const FCompactPoseBoneIndex SourceIndex = SourceBone.GetCompactPoseIndex(BoneContainer);
            if (SourceIndex != INDEX_NONE) CurrentPose.Pose[SourceIndex] = AdditiveIdentity;
        }
        if (bHasOnlyDriveBones)
            for (const FCompactPoseBoneIndex BoneIndex : BonesToFilter) CurrentPose.Pose[BoneIndex] = AdditiveIdentity;
        Output = SourceData;
        FAnimationPoseData AccumulatedAnimationPoseData = Output.GetAnimationPoseData();
        FAnimationRuntime::AccumulateAdditivePose(AccumulatedAnimationPoseData, CurrentAnimationPoseData, 1.f, EAdditiveAnimationType::AAT_LocalSpaceBase);
        return;
    }
    const FAnimationPoseData SourceAnimationPoseData = SourceData.GetAnimationPoseData();
    FAnimationPoseData BlendedAnimationPoseData = Output.GetAnimationPoseData();
    FAnimationRuntime::BlendTwoPosesTogetherPerBone(SourceAnimationPoseData, CurrentAnimationPoseData, BoneBlendWeights, BlendedAnimationPoseData);
}

}
