#include "ue5parse/runtime/Engine/Animation/PoseAsset.h"

#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/Animation/AnimationPoseData.h"
#include "ue5parse/runtime/Engine/Animation/Skeleton.h"
#include "ue5parse/runtime/Engine/Animation/SkeletonRemapping.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"
#include "ue5parse/runtime/Engine/BonePose.h"

namespace ue5parse::runtime {

void FPoseDataContainer::BlendPoseCurve(const FPoseData& PoseData, FBlendedCurve& InOutCurve, float Weight) const {
    FBlendedCurve PoseCurve;
    for (int32 CurveIndex = 0; CurveIndex < Curves.Num(); CurveIndex++) PoseCurve.Set(Curves[CurveIndex].GetName(), PoseData.CurveData[CurveIndex]);
    InOutCurve.Accumulate(PoseCurve, Weight);
}

void UPoseAsset::PostLoad() {
    Super::PostLoad();
    PoseContainer.TrackBoneIndices.Reset();
    if (Skeleton == nullptr) return;
    const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
    PoseContainer.TrackBoneIndices.SetNum(PoseContainer.Tracks.Num());
    for (int32 TrackIndex = 0; TrackIndex < PoseContainer.Tracks.Num(); TrackIndex++)
        PoseContainer.TrackBoneIndices[TrackIndex] = RefSkeleton.FindBoneIndex(PoseContainer.Tracks[TrackIndex]);
}

const TArray<FTransform>& UPoseAsset::GetRetargetTransforms() const {
    if (RetargetSource.isNone() && RetargetSourceAssetReferencePose.Num() > 0) return RetargetSourceAssetReferencePose;
    static const TArray<FTransform> EmptyTransformArray;
    return Skeleton ? Skeleton->GetRefLocalPoses(RetargetSource) : EmptyTransformArray;
}

FName UPoseAsset::GetRetargetTransformsSourceName() const {
    if (RetargetSource.isNone() && RetargetSourceAssetReferencePose.Num() > 0) return GetOutermost()->GetFName();
    return RetargetSource;
}

// The skeleton bone and the compact pose bone of a track; INDEX_NONE when the mesh does not have the bone.
struct FPoseTrackBone {
    int32 SkeletonBoneIndex = INDEX_NONE;
    FCompactPoseBoneIndex CompactBoneIndex = FCompactPoseBoneIndex(INDEX_NONE);
};

static FPoseTrackBone GetTrackBone(const FPoseDataContainer& Container, int32 TrackIndex, const FSkeletonRemapping& SkeletonRemapping,
                                   const FBoneContainer& RequiredBones) {
    const int32 SourceBoneIndex = Container.TrackBoneIndices[TrackIndex];
    const int32 SkeletonBoneIndex = SkeletonRemapping.IsValid() ? SkeletonRemapping.GetTargetSkeletonBoneIndex(SourceBoneIndex) : SourceBoneIndex;
    return {SkeletonBoneIndex, RequiredBones.GetCompactPoseIndexFromSkeletonIndex(SkeletonBoneIndex)};
}

// Adds a weighted additive pose: unlike FTransform's accumulation, the scale deltas add, because the start is the
// additive identity with a zero scale.
static void AccumulateAdditivePoseTransform(FTransform& FinalAtom, const FTransform& SourceAtom, float BlendWeight) {
    FTransform Delta = SourceAtom;
    if (BlendWeight < 1.f - ZERO_ANIMWEIGHT_THRESH) Delta.Blend(FTransform(FQuat::Identity, FVector::ZeroVector, FVector::ZeroVector), Delta, BlendWeight);
    FinalAtom.SetRotation(Delta.GetRotation() * FinalAtom.GetRotation());
    FinalAtom.SetTranslation(FinalAtom.GetTranslation() + Delta.GetTranslation());
    FinalAtom.SetScale3D(FinalAtom.GetScale3D() + Delta.GetScale3D());
    FinalAtom.NormalizeRotation();
}

bool UPoseAsset::GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const {
    const TArray<FPoseCurve>& PoseCurves = ExtractionContext.PoseCurves;
    if (PoseCurves.Num() == 0) return false;
    FCompactPose& OutPose = OutAnimationPoseData.GetPose();
    FBlendedCurve& OutCurve = OutAnimationPoseData.GetCurve();
    const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
    if (bAdditivePose) OutPose.ResetToAdditiveIdentity();
    else OutPose.ResetToRefPose();
    const FSkeletonRemapping& SkeletonRemapping = UE::Anim::FSkeletonRemappingRegistry::Get().GetRemapping(Skeleton, RequiredBones.GetSkeletonAsset());
    const FName RetargetSourceName = GetRetargetTransformsSourceName();
    const TArray<FTransform>& RetargetTransforms = GetRetargetTransforms();
    const int32 TrackNum = PoseContainer.Tracks.Num();

    // One pose (PoseByName): a full pose asset clamps the weight instead of normalizing it.
    if (PoseCurves.Num() == 1) {
        const int32 PoseIndex = PoseCurves[0].PoseIndex;
        if (!PoseContainer.Poses.IsValidIndex(PoseIndex)) return false;
        const float Weight = bAdditivePose ? PoseCurves[0].Value : FMath::Clamp(PoseCurves[0].Value, 0.f, 1.f);
        if (!FAnimWeight::IsRelevant(Weight)) return false;
        const FPoseData& Pose = PoseContainer.Poses[PoseIndex];
        PoseContainer.BlendPoseCurve(Pose, OutCurve, Weight);
        for (int32 TrackIndex = 0; TrackIndex < TrackNum; TrackIndex++) {
            const FPoseTrackBone Bone = GetTrackBone(PoseContainer, TrackIndex, SkeletonRemapping, RequiredBones);
            if (Bone.CompactBoneIndex == INDEX_NONE || !ExtractionContext.IsBoneRequired(Bone.CompactBoneIndex.GetInt())) continue;
            const TArray<FPoseAssetInfluence>& Influences = PoseContainer.TrackPoseInfluenceIndices[TrackIndex].Influences;
            const int32 InfluenceIndex = Influences.IndexOfByPredicate([PoseIndex](const FPoseAssetInfluence& Influence) { return Influence.PoseIndex == PoseIndex; });
            if (InfluenceIndex == INDEX_NONE) continue;
            const FTransform& BonePose = Pose.LocalSpacePose[Influences[InfluenceIndex].BoneTransformIndex];
            FTransform& OutBoneTransform = OutPose[Bone.CompactBoneIndex];
            if (bAdditivePose) {
                AccumulateAdditivePoseTransform(OutBoneTransform, BonePose, Weight);
            } else if (FAnimWeight::IsFullWeight(Weight)) {
                OutBoneTransform = BonePose;
            } else {
                OutBoneTransform = OutBoneTransform * (1.f - Weight);
                OutBoneTransform.AccumulateWithShortestRotation(BonePose, Weight);
            }
            FAnimationRuntime::RetargetBoneTransform(Skeleton, RetargetSourceName, RetargetTransforms, OutBoneTransform, Bone.SkeletonBoneIndex,
                                                     Bone.CompactBoneIndex, RequiredBones, bAdditivePose);
            OutBoneTransform.NormalizeRotation();
        }
        return true;
    }

    // The weighted poses; an additive asset skips its base pose.
    thread_local TArray<float> PoseWeights;
    thread_local TArray<bool> WeightedPoses;
    thread_local TArray<int32> WeightedPoseIndices;
    PoseWeights.Reset();
    PoseWeights.SetNum(PoseContainer.Poses.Num());
    WeightedPoses.Reset();
    WeightedPoses.SetNum(PoseContainer.Poses.Num());
    WeightedPoseIndices.Reset();
    float TotalWeight = 0.f;
    for (const FPoseCurve& Curve : PoseCurves) {
        if (!PoseContainer.Poses.IsValidIndex(Curve.PoseIndex)) continue;
        if ((bAdditivePose && Curve.PoseIndex == BasePoseIndex) || !FAnimWeight::IsRelevant(Curve.Value)) continue;
        TotalWeight += Curve.Value;
        PoseWeights[Curve.PoseIndex] = Curve.Value;
        WeightedPoses[Curve.PoseIndex] = true;
        WeightedPoseIndices.Add(Curve.PoseIndex);
    }
    if (WeightedPoseIndices.Num() == 0) return false;
    const bool bNormalizeWeights = !bAdditivePose && TotalWeight > 1.f;
    for (const int32 PoseIndex : WeightedPoseIndices) {
        if (bNormalizeWeights) PoseWeights[PoseIndex] /= TotalWeight;
        PoseContainer.BlendPoseCurve(PoseContainer.Poses[PoseIndex], OutCurve, PoseWeights[PoseIndex]);
    }

    for (int32 TrackIndex = 0; TrackIndex < TrackNum; TrackIndex++) {
        const FPoseTrackBone Bone = GetTrackBone(PoseContainer, TrackIndex, SkeletonRemapping, RequiredBones);
        if (Bone.CompactBoneIndex == INDEX_NONE || !ExtractionContext.IsBoneRequired(Bone.CompactBoneIndex.GetInt())) continue;
        const FTransform& InputTransform = OutPose[Bone.CompactBoneIndex];
        FTransform OutBoneTransform = InputTransform;
        float TotalLocalWeight = 0.f;
        bool bSet = false;
        for (const FPoseAssetInfluence& Influence : PoseContainer.TrackPoseInfluenceIndices[TrackIndex].Influences) {
            if (!WeightedPoses[Influence.PoseIndex]) continue;
            const float Weight = PoseWeights[Influence.PoseIndex];
            TotalLocalWeight += Weight;
            const FTransform& BonePose = PoseContainer.Poses[Influence.PoseIndex].LocalSpacePose[Influence.BoneTransformIndex];
            if (bAdditivePose) {
                AccumulateAdditivePoseTransform(OutBoneTransform, BonePose, Weight);
            } else if (!bSet) {
                OutBoneTransform = BonePose * Weight;
                bSet = true;
            } else {
                OutBoneTransform.AccumulateWithShortestRotation(BonePose, Weight);
            }
        }
        if (!FAnimWeight::IsRelevant(TotalLocalWeight)) OutBoneTransform = InputTransform;
        else if (!bAdditivePose && !FAnimWeight::IsFullWeight(TotalLocalWeight)) OutBoneTransform.AccumulateWithShortestRotation(InputTransform, 1.f - TotalLocalWeight);
        FAnimationRuntime::RetargetBoneTransform(Skeleton, RetargetSourceName, RetargetTransforms, OutBoneTransform, Bone.SkeletonBoneIndex,
                                                 Bone.CompactBoneIndex, RequiredBones, bAdditivePose);
        OutBoneTransform.NormalizeRotation();
        OutPose[Bone.CompactBoneIndex] = OutBoneTransform;
    }
    return true;
}

}
