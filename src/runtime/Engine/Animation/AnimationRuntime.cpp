#include "ue5parse/runtime/Engine/AnimationRuntime.h"

#include <cmath>

#include "ue5parse/runtime/Engine/Animation/AnimData/BoneMaskFilter.h"
#include "ue5parse/runtime/Engine/Animation/AnimationAsset.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"
#include "ue5parse/runtime/Engine/Animation/BlendProfile.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/Animation/Skeleton.h"
#include "ue5parse/runtime/Engine/Animation/SkeletonRemapping.h"

// UE's AnimationRuntime.cpp (UE 5.3).

namespace ue5parse::runtime {

namespace {
// UE's BONE_TRANS_RT_ORIENT_AND_SCALE_PRECISION.
constexpr double OrientAndScalePrecision = 0.001;
}

void FAnimationRuntime::RetargetBoneTransform(const USkeleton* SourceSkeleton, FName SourceName, const TArray<FTransform>& RetargetTransforms,
                                              FTransform& BoneTransform, int32 SkeletonBoneIndex, const FCompactPoseBoneIndex& BoneIndex,
                                              const FBoneContainer& RequiredBones, bool bIsBakedAdditive) {
    if (SourceSkeleton == nullptr || RetargetTransforms.IsEmpty()) return;
    const USkeleton* TargetSkeleton = RequiredBones.GetSkeletonAsset();
    const FSkeletonRemapping& SkeletonRemapping = UE::Anim::FSkeletonRemappingRegistry::Get().GetRemapping(SourceSkeleton, TargetSkeleton);
    const int32 TargetSkeletonBoneIndex = RequiredBones.GetSkeletonIndex(BoneIndex);
    const int32 SourceSkeletonBoneIndex =
        SkeletonRemapping.IsValid() ? SkeletonRemapping.GetSourceSkeletonBoneIndex(TargetSkeletonBoneIndex) : SkeletonBoneIndex;
    switch (TargetSkeleton->GetBoneTranslationRetargetingMode(TargetSkeletonBoneIndex, RequiredBones.GetDisableRetargeting())) {
    case EBoneTranslationRetargetingMode::AnimationScaled: {
        const float SourceTranslationLength = static_cast<float>(RetargetTransforms[SourceSkeletonBoneIndex].GetTranslation().Size());
        if (SourceTranslationLength > UE_KINDA_SMALL_NUMBER) {
            const float TargetTranslationLength = static_cast<float>(RequiredBones.GetRefPoseTransform(BoneIndex).GetTranslation().Size());
            BoneTransform.ScaleTranslation(TargetTranslationLength / SourceTranslationLength);
        }
        break;
    }
    case EBoneTranslationRetargetingMode::Skeleton:
        BoneTransform.SetTranslation(bIsBakedAdditive ? FVector::ZeroVector : RequiredBones.GetRefPoseTransform(BoneIndex).GetTranslation());
        break;
    case EBoneTranslationRetargetingMode::AnimationRelative:
        // A baked additive cancels the relative offset: (A1 + Rel) - (A2 + Rel) = A1 - A2.
        if (!bIsBakedAdditive) {
            const FTransform& RefPoseTransform = RequiredBones.GetRefPoseTransform(BoneIndex);
            FTransform BaseTransform = RetargetTransforms[SourceSkeletonBoneIndex];
            if (SkeletonRemapping.RequiresReferencePoseRetarget())
                BaseTransform = SkeletonRemapping.RetargetBoneTransformToTargetSkeleton(TargetSkeletonBoneIndex, BaseTransform);
            BoneTransform.SetRotation(BoneTransform.GetRotation() * BaseTransform.GetRotation().Inverse() * RefPoseTransform.GetRotation());
            BoneTransform.SetTranslation(BoneTransform.GetTranslation() + (RefPoseTransform.GetTranslation() - BaseTransform.GetTranslation()));
            BoneTransform.SetScale3D(BoneTransform.GetScale3D() * (RefPoseTransform.GetScale3D() * BaseTransform.GetSafeScaleReciprocal(BaseTransform.GetScale3D())));
            BoneTransform.NormalizeRotation();
        }
        break;
    case EBoneTranslationRetargetingMode::OrientAndScale:
        if (!bIsBakedAdditive) {
            const FRetargetSourceCachedData& CachedData = RequiredBones.GetRetargetSourceCachedData(SourceName, RetargetTransforms);
            if (CachedData.OrientAndScaleData.Num() > 0 && CachedData.CompactPoseIndexToOrientAndScaleIndex.Num() == RequiredBones.GetCompactPoseNumBones()) {
                const int32 OrientAndScaleIndex = CachedData.CompactPoseIndexToOrientAndScaleIndex[BoneIndex.GetInt()];
                if (OrientAndScaleIndex != INDEX_NONE) {
                    const FOrientAndScaleRetargetingCachedData& Data = CachedData.OrientAndScaleData[OrientAndScaleIndex];
                    const FVector AnimatedTranslation = BoneTransform.GetTranslation();
                    BoneTransform.SetTranslation((AnimatedTranslation - Data.SourceTranslation).IsNearlyZero(OrientAndScalePrecision)
                                                     ? Data.TargetTranslation
                                                     : Data.TranslationDeltaOrient.RotateVector(AnimatedTranslation) * Data.TranslationScale);
                }
            }
        }
        break;
    default: break;
    }
}

void FAnimationRuntime::FillUpComponentSpaceTransforms(const FReferenceSkeleton& RefSkeleton, const TArray<FTransform>& BoneSpaceTransforms,
                                                       TArray<FTransform>& ComponentSpaceTransforms) {
    ComponentSpaceTransforms.SetNum(BoneSpaceTransforms.Num());
    if (ComponentSpaceTransforms.Num() == 0) return;
    ComponentSpaceTransforms[0] = BoneSpaceTransforms[0];
    for (int32 BoneIndex = 1; BoneIndex < ComponentSpaceTransforms.Num(); BoneIndex++)
        ComponentSpaceTransforms[BoneIndex] = BoneSpaceTransforms[BoneIndex] * ComponentSpaceTransforms[RefSkeleton.GetParentIndex(BoneIndex)];
}

ETypeAdvanceAnim FAnimationRuntime::AdvanceTime(bool bAllowLooping, float MoveDelta, float& InOutTime, float EndTime) {
    InOutTime += MoveDelta;
    if (InOutTime >= 0.0f && InOutTime <= EndTime) return ETAA_Default;
    if (!bAllowLooping) {
        InOutTime = FMath::Clamp(InOutTime, 0.0f, EndTime);
        return ETAA_Finished;
    }
    if (EndTime == 0.0f) {
        InOutTime = 0.0f;
        return ETAA_Looped;
    }
    InOutTime = std::fmod(InOutTime, EndTime);
    if (InOutTime < 0.0f) InOutTime += EndTime;
    return ETAA_Looped;
}

namespace {

void BlendPoseOverwrite(const FCompactPose& SourcePose, FCompactPose& ResultPose, float BlendWeight) {
    const TArray<FTransform>& Source = SourcePose.GetBones();
    TArray<FTransform>& Result = ResultPose.GetMutableBones();
    for (int32 Index = 0; Index < Source.Num(); ++Index) Result[Index] = Source[Index] * double(BlendWeight);
}

void BlendPoseAccumulate(const FCompactPose& SourcePose, FCompactPose& ResultPose, float BlendWeight) {
    const TArray<FTransform>& Source = SourcePose.GetBones();
    TArray<FTransform>& Result = ResultPose.GetMutableBones();
    for (int32 Index = 0; Index < Source.Num(); ++Index) Result[Index].AccumulateWithShortestRotation(Source[Index], BlendWeight);
}

}

void FAnimationRuntime::BlendPosesTogetherIndirect(std::span<const FCompactPose* const> SourcePoses, std::span<const FBlendedCurve* const> SourceCurves,
                                                   std::span<const float> SourceWeights, FAnimationPoseData& OutAnimationPoseData) {
    FCompactPose& OutPose = OutAnimationPoseData.GetPose();
    BlendPoseOverwrite(*SourcePoses[0], OutPose, SourceWeights[0]);
    for (std::size_t Index = 1; Index < SourcePoses.size(); ++Index) BlendPoseAccumulate(*SourcePoses[Index], OutPose, SourceWeights[Index]);
    if (SourcePoses.size() > 1) OutPose.NormalizeRotations();
    if (!SourceCurves.empty()) BlendCurves(SourceCurves, SourceWeights, OutAnimationPoseData.GetCurve());
}

void FAnimationRuntime::BlendTwoPosesTogether(const FAnimationPoseData& SourcePoseOneData, const FAnimationPoseData& SourcePoseTwoData, float WeightOfPoseOne,
                                              FAnimationPoseData& OutAnimationPoseData) {
    const float WeightOfPoseTwo = 1.0f - WeightOfPoseOne;
    FCompactPose& OutPose = OutAnimationPoseData.GetPose();
    BlendPoseOverwrite(SourcePoseOneData.GetPose(), OutPose, WeightOfPoseOne);
    BlendPoseAccumulate(SourcePoseTwoData.GetPose(), OutPose, WeightOfPoseTwo);
    OutPose.NormalizeRotations();
    OutAnimationPoseData.GetCurve().Lerp(SourcePoseOneData.GetCurve(), SourcePoseTwoData.GetCurve(), WeightOfPoseTwo);
}

void FAnimationRuntime::BlendCurves(std::span<const FBlendedCurve* const> SourceCurves, std::span<const float> SourceWeights, FBlendedCurve& OutCurve) {
    if (SourceCurves.empty()) return;
    OutCurve.Override(*SourceCurves[0], SourceWeights[0]);
    for (std::size_t Index = 1; Index < SourceCurves.size(); ++Index) OutCurve.Accumulate(*SourceCurves[Index], SourceWeights[Index]);
}

namespace {

template <bool bOverwrite>
void BlendPosePerBone(const TArray<int32>& PerBoneIndices, const FBlendSampleData& BlendSampleData, FCompactPose& ResultPose, const FCompactPose& SourcePose) {
    const float BlendWeight = BlendSampleData.GetClampedWeight();
    TArray<FTransform>& Result = ResultPose.GetMutableBones();
    const TArray<FTransform>& Source = SourcePose.GetBones();
    for (int32 Index = 0; Index < Source.Num(); ++Index) {
        const int32 PerBoneIndex = PerBoneIndices[Index];
        const float Weight = BlendSampleData.PerBoneBlendData.IsValidIndex(PerBoneIndex) ? BlendSampleData.PerBoneBlendData[PerBoneIndex] : BlendWeight;
        if (bOverwrite) Result[Index] = Source[Index] * double(Weight);
        else Result[Index].AccumulateWithShortestRotation(Source[Index], Weight);
    }
}

}

void FAnimationRuntime::BlendPosesTogetherPerBone(const TArray<FCompactPose>& SourcePoses, const TArray<FBlendedCurve>& SourceCurves,
                                                  const TArray<int32>& PerBoneIndices, const TArray<FBlendSampleData>& BlendSampleDataCache,
                                                  FAnimationPoseData& OutAnimationPoseData) {
    FCompactPose& OutPose = OutAnimationPoseData.GetPose();
    BlendPosePerBone<true>(PerBoneIndices, BlendSampleDataCache[0], OutPose, SourcePoses[0]);
    for (int32 Index = 1; Index < SourcePoses.Num(); ++Index) BlendPosePerBone<false>(PerBoneIndices, BlendSampleDataCache[Index], OutPose, SourcePoses[Index]);
    OutPose.NormalizeRotations();
    if (SourceCurves.Num() == 0) return;
    TArray<const FBlendedCurve*> Curves;
    TArray<float> SourceWeights;
    Curves.Reserve(SourceCurves.Num());
    SourceWeights.Reserve(BlendSampleDataCache.Num());
    for (const FBlendedCurve& Curve : SourceCurves) Curves.Add(&Curve);
    for (const FBlendSampleData& Sample : BlendSampleDataCache) SourceWeights.Add(Sample.TotalWeight);
    BlendCurves(Curves, SourceWeights, OutAnimationPoseData.GetCurve());
}

void FAnimationRuntime::BlendPosesTogetherPerBoneInMeshSpace(TArray<FCompactPose>& SourcePoses, const TArray<FBlendedCurve>& SourceCurves,
                                                             const TArray<int32>& PerBoneIndices, const TArray<FBlendSampleData>& BlendSampleDataCache,
                                                             FAnimationPoseData& OutAnimationPoseData) {
    for (FCompactPose& Pose : SourcePoses) {
        for (int32 Index = 1; Index < Pose.GetNumBones(); ++Index) {
            const FCompactPoseBoneIndex BoneIndex(Index);
            const FCompactPoseBoneIndex ParentIndex = Pose.GetParentBoneIndex(BoneIndex);
            if (!ParentIndex.IsValid()) continue;
            FQuat NewRotation = Pose[ParentIndex].GetRotation() * Pose[BoneIndex].GetRotation();
            NewRotation.Normalize();
            Pose[BoneIndex].SetRotation(NewRotation);
        }
    }
    BlendPosesTogetherPerBone(SourcePoses, SourceCurves, PerBoneIndices, BlendSampleDataCache, OutAnimationPoseData);
    FCompactPose& OutPose = OutAnimationPoseData.GetPose();
    for (int32 Index = OutPose.GetNumBones() - 1; Index > 0; --Index) {
        const FCompactPoseBoneIndex BoneIndex(Index);
        const FCompactPoseBoneIndex ParentIndex = OutPose.GetParentBoneIndex(BoneIndex);
        if (!ParentIndex.IsValid()) continue;
        OutPose[BoneIndex].SetRotation(OutPose[ParentIndex].GetRotation().Inverse() * OutPose[BoneIndex].GetRotation());
        OutPose[BoneIndex].NormalizeRotation();
    }
}

void FAnimationRuntime::BlendTwoPosesTogetherPerBone(const FAnimationPoseData& SourcePoseOneData, const FAnimationPoseData& SourcePoseTwoData,
                                                     const TArray<float>& WeightsOfSource2, FAnimationPoseData& OutAnimationPoseData) {
    FCompactPose& OutPose = OutAnimationPoseData.GetPose();
    const FCompactPose& SourcePoseOne = SourcePoseOneData.GetPose();
    const FCompactPose& SourcePoseTwo = SourcePoseTwoData.GetPose();
    for (int32 Index = 0; Index < OutPose.GetNumBones(); ++Index) {
        const FCompactPoseBoneIndex BoneIndex(Index);
        const float BlendWeight = WeightsOfSource2[Index];
        if (BlendWeight >= 1.f - ZERO_ANIMWEIGHT_THRESH) {
            OutPose[BoneIndex] = SourcePoseTwo[BoneIndex];
        } else if (BlendWeight > ZERO_ANIMWEIGHT_THRESH) {
            OutPose[BoneIndex] = SourcePoseOne[BoneIndex] * double(1.f - BlendWeight);
            OutPose[BoneIndex].AccumulateWithShortestRotation(SourcePoseTwo[BoneIndex], BlendWeight);
        } else {
            OutPose[BoneIndex] = SourcePoseOne[BoneIndex];
        }
    }
    OutPose.NormalizeRotations();
    OutAnimationPoseData.GetCurve().Override(SourcePoseOneData.GetCurve());
    OutAnimationPoseData.GetCurve().Combine(SourcePoseTwoData.GetCurve());
}

void FAnimationRuntime::BlendCurves(std::span<const FBlendedCurve* const> SourceCurves, std::span<const float> SourceWeights, FBlendedCurve& OutCurve,
                                    ECurveBlendOption::Type BlendOption) {
    if (SourceCurves.empty()) return;
    switch (BlendOption) {
        case ECurveBlendOption::BlendByWeight: BlendCurves(SourceCurves, SourceWeights, OutCurve); return;
        case ECurveBlendOption::NormalizeByWeight: {
            float SumOfWeight = 0.f;
            for (const float Weight : SourceWeights) SumOfWeight += Weight;
            if (!FAnimWeight::IsRelevant(SumOfWeight)) {
                BlendCurves(SourceCurves, SourceWeights, OutCurve);
                return;
            }
            TArray<float> NormalizedSourceWeights;
            NormalizedSourceWeights.Reserve(static_cast<int32>(SourceWeights.size()));
            for (const float Weight : SourceWeights) NormalizedSourceWeights.Add(Weight / SumOfWeight);
            BlendCurves(SourceCurves, NormalizedSourceWeights, OutCurve);
            return;
        }
        default: break;
    }
    OutCurve.Override(*SourceCurves[0], SourceWeights[0]);
    for (std::size_t Index = 1; Index < SourceCurves.size(); ++Index) {
        switch (BlendOption) {
            case ECurveBlendOption::UseMaxValue: OutCurve.UseMaxValue(*SourceCurves[Index]); break;
            case ECurveBlendOption::UseMinValue: OutCurve.UseMinValue(*SourceCurves[Index]); break;
            case ECurveBlendOption::UseBasePose: return;
            case ECurveBlendOption::DoNotOverride: OutCurve.CombinePreserved(*SourceCurves[Index]); break;
            default: OutCurve.Combine(*SourceCurves[Index]); break;
        }
    }
}

void FAnimationRuntime::UpdateDesiredBoneWeight(const TArray<FPerBoneBlendWeight>& SrcBoneBlendWeights, TArray<FPerBoneBlendWeight>& TargetBoneBlendWeights,
                                                const TArray<float>& BlendWeights) {
    for (FPerBoneBlendWeight& Weight : TargetBoneBlendWeights) Weight = FPerBoneBlendWeight();
    for (int32 BoneIndex = 0; BoneIndex < SrcBoneBlendWeights.Num(); ++BoneIndex) {
        const int32 PoseIndex = SrcBoneBlendWeights[BoneIndex].SourceIndex;
        const float TargetBlendWeight = BlendWeights[PoseIndex] * SrcBoneBlendWeights[BoneIndex].BlendWeight;
        if (!FAnimWeight::IsRelevant(TargetBlendWeight)) continue;
        TargetBoneBlendWeights[BoneIndex].SourceIndex = PoseIndex;
        TargetBoneBlendWeights[BoneIndex].BlendWeight = TargetBlendWeight;
    }
}

void FAnimationRuntime::BlendPosesPerBoneFilter(FCompactPose& BasePose, const TArray<FCompactPose>& BlendPoses, FBlendedCurve& BaseCurve,
                                                const TArray<FBlendedCurve>& BlendedCurves, FAnimationPoseData& OutAnimationPoseData,
                                                const TArray<FPerBoneBlendWeight>& BoneBlendWeights, EBlendPosesPerBoneFilterFlags BlendFlags,
                                                ECurveBlendOption::Type CurveBlendOption) {
    FCompactPose& OutPose = OutAnimationPoseData.GetPose();
    if (BlendPoses.Num() == 0) {
        OutPose.CopyBonesFrom(BasePose);
        return;
    }
    const int32 NumBones = BasePose.GetNumBones();
    const int32 NumPoses = BlendPoses.Num();
    TArray<float> MaxPoseWeights;
    MaxPoseWeights.SetNum(NumPoses);
    for (float& Weight : MaxPoseWeights) Weight = 0.f;

    const bool bMeshSpaceRotationBlend = EnumHasAnyFlags(BlendFlags, EBlendPosesPerBoneFilterFlags::MeshSpaceRotation);
    const bool bMeshSpaceScaleBlend = EnumHasAnyFlags(BlendFlags, EBlendPosesPerBoneFilterFlags::MeshSpaceScale);
    TArray<FQuat> SourceRotations, BlendRotations, TargetRotations;
    TArray<FVector> SourceScales, BlendScales, TargetScales;
    if (bMeshSpaceRotationBlend) {
        SourceRotations.SetNum(NumBones);
        BlendRotations.SetNum(NumBones);
        TargetRotations.SetNum(NumBones);
    }
    if (bMeshSpaceScaleBlend) {
        SourceScales.SetNum(NumBones);
        BlendScales.SetNum(NumBones);
        TargetScales.SetNum(NumBones);
    }

    const FBoneContainer& BoneContainer = BasePose.GetBoneContainer();
    for (int32 Index = 0; Index < NumBones; ++Index) {
        const FCompactPoseBoneIndex BoneIndex(Index);
        const int32 PoseIndex = BoneBlendWeights[Index].SourceIndex;
        const FCompactPoseBoneIndex ParentIndex = BoneContainer.GetParentBoneIndex(BoneIndex);
        const bool bHasParent = ParentIndex.IsValid();
        if (bMeshSpaceRotationBlend) {
            SourceRotations[Index] = (bHasParent ? SourceRotations[ParentIndex.GetInt()] : FQuat::Identity) * BasePose[BoneIndex].GetRotation();
            TargetRotations[Index] = (bHasParent ? TargetRotations[ParentIndex.GetInt()] : FQuat::Identity) * BlendPoses[PoseIndex][BoneIndex].GetRotation();
        }
        if (bMeshSpaceScaleBlend) {
            SourceScales[Index] = (bHasParent ? SourceScales[ParentIndex.GetInt()] : FVector(1.0)) * BasePose[BoneIndex].GetScale3D();
            TargetScales[Index] = (bHasParent ? TargetScales[ParentIndex.GetInt()] : FVector(1.0)) * BlendPoses[PoseIndex][BoneIndex].GetScale3D();
        }

        const FTransform& BaseAtom = BasePose[BoneIndex];
        const FTransform& TargetAtom = BlendPoses[PoseIndex][BoneIndex];
        FTransform BlendAtom;
        const float BlendWeight = FMath::Clamp(BoneBlendWeights[Index].BlendWeight, 0.f, 1.f);
        MaxPoseWeights[PoseIndex] = FMath::Max(MaxPoseWeights[PoseIndex], BlendWeight);
        if (!FAnimWeight::IsRelevant(BlendWeight)) {
            BlendAtom = BaseAtom;
            if (bMeshSpaceRotationBlend) BlendRotations[Index] = SourceRotations[Index];
            if (bMeshSpaceScaleBlend) BlendScales[Index] = SourceScales[Index];
        } else if (FAnimWeight::IsFullWeight(BlendWeight)) {
            BlendAtom = TargetAtom;
            if (bMeshSpaceRotationBlend) BlendRotations[Index] = TargetRotations[Index];
            if (bMeshSpaceScaleBlend) BlendScales[Index] = TargetScales[Index];
        } else {
            BlendAtom = BaseAtom;
            BlendAtom.BlendWith(TargetAtom, BlendWeight);
            if (bMeshSpaceRotationBlend) {
                BlendRotations[Index] = FQuat::FastLerp(SourceRotations[Index], TargetRotations[Index], BlendWeight);
                BlendRotations[Index].Normalize();
            }
            if (bMeshSpaceScaleBlend) BlendScales[Index] = FMath::Lerp(SourceScales[Index], TargetScales[Index], double(BlendWeight));
        }
        if (bHasParent && bMeshSpaceRotationBlend) {
            // Local to mesh to local loses precision on long chains: UE normalizes here.
            FQuat LocalBlendQuat = BlendRotations[ParentIndex.GetInt()].Inverse() * BlendRotations[Index];
            LocalBlendQuat.Normalize();
            BlendAtom.SetRotation(LocalBlendQuat);
        }
        if (bHasParent && bMeshSpaceScaleBlend)
            BlendAtom.SetScale3D(FTransform::GetSafeScaleReciprocal(BlendScales[ParentIndex.GetInt()], UE_SMALL_NUMBER) * BlendScales[Index]);
        OutPose[BoneIndex] = BlendAtom;
    }

    // Each pose's curves blend by the highest weight of its bones.
    TArray<const FBlendedCurve*> SourceCurves;
    TArray<float> SourceWeights;
    SourceCurves.Reserve(NumPoses + 1);
    SourceWeights.Reserve(NumPoses + 1);
    SourceCurves.Add(&BaseCurve);
    SourceWeights.Add(1.f);
    for (int32 Idx = 0; Idx < NumPoses; ++Idx) {
        SourceCurves.Add(&BlendedCurves[Idx]);
        SourceWeights.Add(MaxPoseWeights[Idx]);
    }
    BlendCurves(SourceCurves, SourceWeights, OutAnimationPoseData.GetCurve(), CurveBlendOption);
}

void FAnimationRuntime::CreateMaskWeights(TArray<FPerBoneBlendWeight>& BoneBlendWeights, const TArray<FInputBlendPose>& BlendFilters,
                                          const USkeleton* Skeleton) {
    if (Skeleton == nullptr) return;
    const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
    const int32 NumBones = RefSkeleton.GetNum();
    BoneBlendWeights.Reset();
    BoneBlendWeights.SetNum(NumBones);
    for (int32 PoseIndex = 0; PoseIndex < BlendFilters.Num(); ++PoseIndex) {
        for (const FBranchFilter& BranchFilter : BlendFilters[PoseIndex].BranchFilters) {
            const int32 MaskBoneIndex = RefSkeleton.FindBoneIndex(BranchFilter.BoneName);
            if (MaskBoneIndex == INDEX_NONE) continue;
            const float IncreaseWeightPerDepth = BranchFilter.BlendDepth != 0 ? 1.f / static_cast<float>(BranchFilter.BlendDepth) : 1.f;
            // Parents come before their children, so the children of the mask bone follow it.
            for (int32 BoneIndex = MaskBoneIndex; BoneIndex < NumBones; ++BoneIndex) {
                const int32 Depth = RefSkeleton.GetDepthBetweenBones(BoneIndex, MaskBoneIndex);
                if (Depth == -1) continue;
                FPerBoneBlendWeight& BoneBlendWeight = BoneBlendWeights[BoneIndex];
                BoneBlendWeight.SourceIndex = PoseIndex;
                BoneBlendWeight.BlendWeight = FMath::Clamp(BoneBlendWeight.BlendWeight + IncreaseWeightPerDepth * static_cast<float>(Depth + 1), 0.f, 1.f);
            }
        }
    }
}

void FAnimationRuntime::CreateMaskWeights(TArray<FPerBoneBlendWeight>& BoneBlendWeights, const TArray<TObjectPtr<UBlendProfile>>& BlendMasks,
                                          const USkeleton* Skeleton) {
    if (Skeleton == nullptr) return;
    const int32 NumBones = Skeleton->GetReferenceSkeleton().GetNum();
    BoneBlendWeights.Reset();
    BoneBlendWeights.SetNum(NumBones);
    for (int32 MaskIndex = 0; MaskIndex < BlendMasks.Num(); ++MaskIndex) {
        const UBlendProfile* BlendMask = BlendMasks[MaskIndex];
        if (BlendMask == nullptr || BlendMask->Mode != EBlendProfileMode::BlendMask) continue;
        const FSkeletonRemapping& SkeletonRemapping = UE::Anim::FSkeletonRemappingRegistry::Get().GetRemapping(BlendMask->OwningSkeleton, Skeleton);
        for (const FBlendProfileBoneEntry& Entry : BlendMask->ProfileEntries) {
            // The entry's cooked bone index is of the mask's skeleton.
            FBoneReference BoneReference = Entry.BoneReference;
            BoneReference.Initialize(BlendMask->OwningSkeleton);
            const int32 BoneIndex =
                SkeletonRemapping.IsValid() ? SkeletonRemapping.GetTargetSkeletonBoneIndex(BoneReference.BoneIndex) : BoneReference.BoneIndex;
            if (!BoneBlendWeights.IsValidIndex(BoneIndex)) continue;
            BoneBlendWeights[BoneIndex].SourceIndex = MaskIndex;
            BoneBlendWeights[BoneIndex].BlendWeight = Entry.BlendScale;
        }
    }
}

void FAnimationRuntime::LerpPoses(FCompactPose& PoseA, const FCompactPose& PoseB, FBlendedCurve& CurveA, const FBlendedCurve& CurveB, float Alpha) {
    if (!FAnimWeight::IsRelevant(Alpha)) return;
    if (FAnimWeight::IsFullWeight(Alpha)) {
        PoseA.CopyBonesFrom(PoseB);
        CurveA.CopyFrom(CurveB);
        return;
    }
    TArray<FTransform>& BonesA = PoseA.GetMutableBones();
    const TArray<FTransform>& BonesB = PoseB.GetBones();
    for (int32 Index = 0; Index < BonesA.Num(); ++Index) {
        BonesA[Index] *= double(1.0f - Alpha);
        BonesA[Index].AccumulateWithShortestRotation(BonesB[Index], Alpha);
        BonesA[Index].NormalizeRotation();
    }
    CurveA.LerpTo(CurveB, Alpha);
}

void FAnimationRuntime::LerpPosesPerBone(FCompactPose& PoseA, const FCompactPose& PoseB, FBlendedCurve& CurveA, const FBlendedCurve& CurveB, float Alpha,
                                         const TArray<float>& PerBoneWeights) {
    if (!FAnimWeight::IsRelevant(Alpha)) return;
    TArray<FTransform>& BonesA = PoseA.GetMutableBones();
    const TArray<FTransform>& BonesB = PoseB.GetBones();
    for (int32 Index = 0; Index < BonesA.Num(); ++Index) {
        const float BoneAlpha = Alpha * PerBoneWeights[Index];
        if (!FAnimWeight::IsRelevant(BoneAlpha)) continue;
        BonesA[Index] *= double(1.0f - BoneAlpha);
        BonesA[Index].AccumulateWithShortestRotation(BonesB[Index], BoneAlpha);
        BonesA[Index].NormalizeRotation();
    }
    CurveA.LerpTo(CurveB, Alpha);
}

void FAnimationRuntime::ConvertTransformToAdditive(FTransform& TargetTransform, const FTransform& BaseTransform) {
    TargetTransform.SetRotation(TargetTransform.GetRotation() * BaseTransform.GetRotation().Inverse());
    TargetTransform.SetTranslation(TargetTransform.GetTranslation() - BaseTransform.GetTranslation());
    // An additive scale is the ratio minus 1, so that deltas blend; it applies as scale * (1 + delta).
    TargetTransform.SetScale3D(TargetTransform.GetScale3D() * FTransform::GetSafeScaleReciprocal(BaseTransform.GetScale3D()) - 1.0);
    TargetTransform.NormalizeRotation();
}

void FAnimationRuntime::ConvertPoseToAdditive(FCompactPose& TargetPose, const FCompactPose& BasePose) {
    TArray<FTransform>& Target = TargetPose.GetMutableBones();
    const TArray<FTransform>& Base = BasePose.GetBones();
    for (int32 Index = 0; Index < Target.Num(); ++Index) ConvertTransformToAdditive(Target[Index], Base[Index]);
}

void FAnimationRuntime::ConvertPoseToMeshRotation(FCompactPose& LocalPose) {
    for (int32 Index = 1; Index < LocalPose.GetNumBones(); ++Index) {
        const FCompactPoseBoneIndex BoneIndex(Index);
        const FCompactPoseBoneIndex ParentIndex = LocalPose.GetParentBoneIndex(BoneIndex);
        LocalPose[BoneIndex].SetRotation(LocalPose[ParentIndex].GetRotation() * LocalPose[BoneIndex].GetRotation());
    }
}

void FAnimationRuntime::ConvertMeshRotationPoseToLocalSpace(FCompactPose& Pose) {
    for (int32 Index = Pose.GetNumBones() - 1; Index > 0; --Index) {
        const FCompactPoseBoneIndex BoneIndex(Index);
        const FCompactPoseBoneIndex ParentIndex = Pose.GetParentBoneIndex(BoneIndex);
        Pose[BoneIndex].SetRotation(Pose[ParentIndex].GetRotation().Inverse() * Pose[BoneIndex].GetRotation());
    }
}

void FAnimationRuntime::AccumulateLocalSpaceAdditivePoseInternal(FCompactPose& BasePose, const FCompactPose& AdditivePose, float Weight) {
    if (!FAnimWeight::IsRelevant(Weight)) return;
    TArray<FTransform>& Base = BasePose.GetMutableBones();
    const TArray<FTransform>& Additive = AdditivePose.GetBones();
    if (FAnimWeight::IsFullWeight(Weight)) {
        for (int32 Index = 0; Index < Base.Num(); ++Index) Base[Index].AccumulateWithAdditiveScale(Additive[Index], Weight);
        return;
    }
    for (int32 Index = 0; Index < Base.Num(); ++Index) FTransform::BlendFromIdentityAndAccumulate(Base[Index], Additive[Index], Weight);
}

void FAnimationRuntime::AccumulateMeshSpaceRotationAdditiveToLocalPoseInternal(FCompactPose& BasePose, const FCompactPose& MeshSpaceRotationAdditive,
                                                                               float Weight) {
    if (!FAnimWeight::IsRelevant(Weight)) return;
    ConvertPoseToMeshRotation(BasePose);
    AccumulateLocalSpaceAdditivePoseInternal(BasePose, MeshSpaceRotationAdditive, Weight);
    ConvertMeshRotationPoseToLocalSpace(BasePose);
}

void FAnimationRuntime::AccumulateAdditivePose(FAnimationPoseData& BaseAnimationPoseData, const FAnimationPoseData& AdditiveAnimationPoseData, float Weight,
                                               EAdditiveAnimationType AdditiveType) {
    if (AdditiveType == AAT_RotationOffsetMeshSpace)
        AccumulateMeshSpaceRotationAdditiveToLocalPoseInternal(BaseAnimationPoseData.GetPose(), AdditiveAnimationPoseData.GetPose(), Weight);
    else
        AccumulateLocalSpaceAdditivePoseInternal(BaseAnimationPoseData.GetPose(), AdditiveAnimationPoseData.GetPose(), Weight);
    BaseAnimationPoseData.GetCurve().Accumulate(AdditiveAnimationPoseData.GetCurve(), Weight);
    BaseAnimationPoseData.GetPose().NormalizeRotations();
}

void FAnimationRuntime::ConvertCSTransformToBoneSpace(const FTransform& ComponentTransform, FCSPose<FCompactPose>& MeshBases,
                                                      FTransform& InOutCSBoneTM, FCompactPoseBoneIndex BoneIndex, EBoneControlSpace Space) {
    switch (Space) {
        case BCS_WorldSpace: InOutCSBoneTM *= ComponentTransform; break;
        case BCS_ComponentSpace: break;
        case BCS_ParentBoneSpace: {
            const FCompactPoseBoneIndex ParentIndex = MeshBases.GetPose().GetParentBoneIndex(BoneIndex);
            if (ParentIndex.IsValid()) InOutCSBoneTM.SetToRelativeTransform(MeshBases.GetComponentSpaceTransform(ParentIndex));
            break;
        }
        case BCS_BoneSpace: InOutCSBoneTM.SetToRelativeTransform(MeshBases.GetComponentSpaceTransform(BoneIndex)); break;
    }
}

FTransform FAnimationRuntime::GetComponentSpaceRefPose(const FCompactPoseBoneIndex& CompactPoseBoneIndex, const FBoneContainer& BoneContainer) {
    FCompactPoseBoneIndex CurrentIndex = CompactPoseBoneIndex;
    FTransform CSTransform = FTransform::Identity;
    while (CurrentIndex != INDEX_NONE) {
        CSTransform *= BoneContainer.GetRefPoseTransform(CurrentIndex);
        CurrentIndex = BoneContainer.GetParentBoneIndex(CurrentIndex);
    }
    return CSTransform;
}

void FAnimationRuntime::ConvertBoneSpaceTransformToCS(const FTransform& ComponentTransform, FCSPose<FCompactPose>& MeshBases,
                                                      FTransform& InOutBoneSpaceTM, FCompactPoseBoneIndex BoneIndex, EBoneControlSpace Space) {
    switch (Space) {
        case BCS_WorldSpace: InOutBoneSpaceTM.SetToRelativeTransform(ComponentTransform); break;
        case BCS_ComponentSpace: break;
        case BCS_ParentBoneSpace: {
            if (!BoneIndex.IsValid()) break;
            const FCompactPoseBoneIndex ParentIndex = MeshBases.GetPose().GetParentBoneIndex(BoneIndex);
            if (ParentIndex.IsValid()) InOutBoneSpaceTM *= MeshBases.GetComponentSpaceTransform(ParentIndex);
            break;
        }
        case BCS_BoneSpace:
            if (BoneIndex.IsValid()) InOutBoneSpaceTM *= MeshBases.GetComponentSpaceTransform(BoneIndex);
            break;
    }
}

}
