#include "ue5parse/runtime/Engine/Animation/AnimationDecompression.h"

#include "ue5parse/runtime/Engine/Animation/AnimBoneCompressionCodec.h"
#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"
#include "ue5parse/runtime/Engine/Animation/Skeleton.h"
#include "ue5parse/runtime/Engine/Animation/SkeletonRemapping.h"
#include "ue5parse/runtime/Engine/AnimationRuntime.h"

// UE's AnimationDecompression.cpp (UE 5.3).

namespace ue5parse::runtime {

namespace {
// UE's BONE_TRANS_RT_ORIENT_AND_SCALE_PRECISION.
constexpr double OrientAndScalePrecision = 0.001;
}

double FAnimSequenceDecompressionContext::GetInterpolatedEvaluationTime() const {
    const double Time = Interpolation == EAnimInterpolationType::Step ? std::floor(SamplingTime) : SamplingTime;
    return Time * SamplingRate.AsInterval();
}

void FAnimSequenceDecompressionContext::Seek(double SampleAtTime) {
    // FFrameRate::AsFrameTime: the frame number and a float sub-frame below 1.
    const double TimeAsFrame = SampleAtTime * SamplingRate.Numerator / SamplingRate.Denominator;
    const double Frame = std::floor(TimeAsFrame);
    const float SubFrame = FMath::Min(static_cast<float>(TimeAsFrame - Frame), 0.99999994f);
    SamplingTime = Frame + SubFrame;
}

void FRootMotionReset::ResetRootBoneForRootMotion(FTransform& BoneTransform, const FBoneContainer& RequiredBones) const {
    switch (RootMotionRootLock) {
    case ERootMotionRootLock::AnimFirstFrame: BoneTransform = AnimFirstFrame; break;
    case ERootMotionRootLock::Zero: BoneTransform = FTransform::Identity; break;
    default: BoneTransform = RequiredBones.GetRefPoseArray()[0]; break;
    }
    // An additive root's scale is a delta.
    if (bIsValidAdditive && RootMotionRootLock != ERootMotionRootLock::AnimFirstFrame)
        BoneTransform.SetScale3D(BoneTransform.GetScale3D() - FVector(1.0));
}

namespace UE::Anim::Decompression {

void DecompressPose(FCompactPose& OutPose, const FCompressedAnimSequence& CompressedData, const FAnimExtractContext& ExtractionContext,
                    FAnimSequenceDecompressionContext& DecompressionContext, const TArray<FTransform>& RetargetTransforms,
                    const FRootMotionReset& RootMotionReset) {
    const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
    const int32 NumTracks = CompressedData.CompressedTrackToSkeletonMapTable.Num();
    const USkeleton* TargetSkeleton = RequiredBones.GetSkeletonAsset();
    const FSkeletonRemapping& SkeletonRemapping =
        FSkeletonRemappingRegistry::Get().GetRemapping(DecompressionContext.GetSourceSkeleton(), TargetSkeleton);
    const bool bAdditive = DecompressionContext.IsAdditiveAnimation();
    const bool bDisableRetargeting = RequiredBones.GetDisableRetargeting();

    thread_local BoneTrackArray RotationScalePairs;
    thread_local BoneTrackArray TranslationPairs;
    thread_local BoneTrackArray AnimScaleRetargetingPairs;
    thread_local BoneTrackArray AnimRelativeRetargetingPairs;
    thread_local BoneTrackArray OrientAndScaleRetargetingPairs;
    RotationScalePairs.Reset();
    TranslationPairs.Reset();
    AnimScaleRetargetingPairs.Reset();
    AnimRelativeRetargetingPairs.Reset();
    OrientAndScaleRetargetingPairs.Reset();

    const bool bFirstTrackIsRootBone = NumTracks > 0 && CompressedData.GetSkeletonIndexFromTrackIndex(0) == 0;
    for (int32 TrackIndex = bFirstTrackIsRootBone ? 1 : 0; TrackIndex < NumTracks; TrackIndex++) {
        const int32 SourceSkeletonBoneIndex = CompressedData.GetSkeletonIndexFromTrackIndex(TrackIndex);
        const int32 TargetSkeletonBoneIndex =
            SkeletonRemapping.IsValid() ? SkeletonRemapping.GetTargetSkeletonBoneIndex(SourceSkeletonBoneIndex) : SourceSkeletonBoneIndex;
        if (TargetSkeletonBoneIndex == INDEX_NONE) continue;
        const int32 CompactPoseBoneIndex = RequiredBones.GetCompactPoseIndexFromSkeletonIndex(TargetSkeletonBoneIndex).GetInt();
        if (CompactPoseBoneIndex == INDEX_NONE) continue;
        RotationScalePairs.Add({CompactPoseBoneIndex, TrackIndex});
        switch (TargetSkeleton->GetBoneTranslationRetargetingMode(TargetSkeletonBoneIndex, bDisableRetargeting)) {
        case EBoneTranslationRetargetingMode::Animation: TranslationPairs.Add({CompactPoseBoneIndex, TrackIndex}); break;
        case EBoneTranslationRetargetingMode::AnimationScaled:
            TranslationPairs.Add({CompactPoseBoneIndex, TrackIndex});
            AnimScaleRetargetingPairs.Add({CompactPoseBoneIndex, SourceSkeletonBoneIndex});
            break;
        case EBoneTranslationRetargetingMode::AnimationRelative:
            TranslationPairs.Add({CompactPoseBoneIndex, TrackIndex});
            if (!bAdditive) AnimRelativeRetargetingPairs.Add({CompactPoseBoneIndex, SourceSkeletonBoneIndex});
            break;
        case EBoneTranslationRetargetingMode::OrientAndScale:
            TranslationPairs.Add({CompactPoseBoneIndex, TrackIndex});
            if (!bAdditive) OrientAndScaleRetargetingPairs.Add({CompactPoseBoneIndex, SourceSkeletonBoneIndex});
            break;
        default: break;
        }
    }

    DecompressionContext.Seek(ExtractionContext.CurrentTime);
    const auto KeepsTranslation = [&](int32 TargetSkeletonBoneIndex) {
        return TargetSkeleton->GetBoneTranslationRetargetingMode(TargetSkeletonBoneIndex, bDisableRetargeting) != EBoneTranslationRetargetingMode::Skeleton;
    };
    if (bFirstTrackIsRootBone) {
        const FCompactPoseBoneIndex RootBone(0);
        FTransform& RootAtom = OutPose[RootBone];
        CompressedData.BoneCompressionCodec->DecompressBone(DecompressionContext, 0, RootAtom);
        if (SkeletonRemapping.RequiresReferencePoseRetarget()) {
            if (bAdditive) {
                RootAtom.SetRotation(SkeletonRemapping.RetargetAdditiveRotationToTargetSkeleton(0, RootAtom.GetRotation()));
                if (KeepsTranslation(0)) RootAtom.SetTranslation(SkeletonRemapping.RetargetAdditiveTranslationToTargetSkeleton(0, RootAtom.GetTranslation()));
            } else {
                RootAtom.SetRotation(SkeletonRemapping.RetargetBoneRotationToTargetSkeleton(0, RootAtom.GetRotation()));
                if (KeepsTranslation(0)) RootAtom.SetTranslation(SkeletonRemapping.RetargetBoneTranslationToTargetSkeleton(0, RootAtom.GetTranslation()));
            }
        }
        FAnimationRuntime::RetargetBoneTransform(DecompressionContext.GetSourceSkeleton(), DecompressionContext.AnimName, RetargetTransforms,
                                                 RootAtom, 0, RootBone, RequiredBones, bAdditive);
    }
    if (RotationScalePairs.Num() > 0)
        CompressedData.BoneCompressionCodec->DecompressPose(DecompressionContext, RotationScalePairs, TranslationPairs, RotationScalePairs,
                                                            OutPose.GetMutableBones());

    if (SkeletonRemapping.RequiresReferencePoseRetarget()) {
        for (FCompactPoseBoneIndex BoneIndex(bFirstTrackIsRootBone ? 1 : 0); BoneIndex < OutPose.GetNumBones(); ++BoneIndex) {
            const int32 TargetSkeletonBoneIndex = RequiredBones.GetSkeletonIndex(BoneIndex);
            FTransform& Bone = OutPose[BoneIndex];
            if (bAdditive) {
                Bone.SetRotation(SkeletonRemapping.RetargetAdditiveRotationToTargetSkeleton(TargetSkeletonBoneIndex, Bone.GetRotation()));
                if (KeepsTranslation(TargetSkeletonBoneIndex))
                    Bone.SetTranslation(SkeletonRemapping.RetargetAdditiveTranslationToTargetSkeleton(TargetSkeletonBoneIndex, Bone.GetTranslation()));
            } else {
                Bone.SetRotation(SkeletonRemapping.RetargetBoneRotationToTargetSkeleton(TargetSkeletonBoneIndex, Bone.GetRotation()));
                if (KeepsTranslation(TargetSkeletonBoneIndex))
                    Bone.SetTranslation(SkeletonRemapping.RetargetBoneTranslationToTargetSkeleton(TargetSkeletonBoneIndex, Bone.GetTranslation()));
            }
        }
    }

    if ((ExtractionContext.bExtractRootMotion && RootMotionReset.bEnableRootMotion) || RootMotionReset.bForceRootLock)
        RootMotionReset.ResetRootBoneForRootMotion(OutPose[FCompactPoseBoneIndex(0)], RequiredBones);

    for (const BoneTrackPair& BonePair : AnimScaleRetargetingPairs) {
        const FCompactPoseBoneIndex BoneIndex(BonePair.AtomIndex);
        const float SourceTranslationLength = static_cast<float>(RetargetTransforms[BonePair.TrackIndex].GetTranslation().Size());
        if (SourceTranslationLength > UE_KINDA_SMALL_NUMBER) {
            const float TargetTranslationLength = static_cast<float>(RequiredBones.GetRefPoseTransform(BoneIndex).GetTranslation().Size());
            OutPose[BoneIndex].ScaleTranslation(TargetTranslationLength / SourceTranslationLength);
        }
    }
    for (const BoneTrackPair& BonePair : AnimRelativeRetargetingPairs) {
        const FCompactPoseBoneIndex BoneIndex(BonePair.AtomIndex);
        const FTransform& RefPoseTransform = RequiredBones.GetRefPoseTransform(BoneIndex);
        FTransform BaseTransform = RetargetTransforms[BonePair.TrackIndex];
        if (SkeletonRemapping.RequiresReferencePoseRetarget())
            BaseTransform = SkeletonRemapping.RetargetBoneTransformToTargetSkeleton(SkeletonRemapping.GetTargetSkeletonBoneIndex(BonePair.TrackIndex),
                                                                                    BaseTransform);
        FTransform& Bone = OutPose[BoneIndex];
        Bone.SetRotation(Bone.GetRotation() * BaseTransform.GetRotation().Inverse() * RefPoseTransform.GetRotation());
        Bone.SetTranslation(Bone.GetTranslation() + (RefPoseTransform.GetTranslation() - BaseTransform.GetTranslation()));
        Bone.SetScale3D(Bone.GetScale3D() * (RefPoseTransform.GetScale3D() * BaseTransform.GetSafeScaleReciprocal(BaseTransform.GetScale3D())));
        Bone.NormalizeRotation();
    }
    if (OrientAndScaleRetargetingPairs.Num() > 0) {
        const FRetargetSourceCachedData& CachedData = RequiredBones.GetRetargetSourceCachedData(DecompressionContext.AnimName, RetargetTransforms);
        if (CachedData.OrientAndScaleData.Num() > 0 && CachedData.CompactPoseIndexToOrientAndScaleIndex.Num() == RequiredBones.GetCompactPoseNumBones()) {
            for (const BoneTrackPair& BonePair : OrientAndScaleRetargetingPairs) {
                const int32 OrientAndScaleIndex = CachedData.CompactPoseIndexToOrientAndScaleIndex[BonePair.AtomIndex];
                if (OrientAndScaleIndex == INDEX_NONE) continue;
                const FOrientAndScaleRetargetingCachedData& Data = CachedData.OrientAndScaleData[OrientAndScaleIndex];
                FTransform& BoneTransform = OutPose[FCompactPoseBoneIndex(BonePair.AtomIndex)];
                const FVector AnimatedTranslation = BoneTransform.GetTranslation();
                BoneTransform.SetTranslation((AnimatedTranslation - Data.SourceTranslation).IsNearlyZero(OrientAndScalePrecision)
                                                 ? Data.TargetTranslation
                                                 : Data.TranslationDeltaOrient.RotateVector(AnimatedTranslation) * Data.TranslationScale);
            }
        }
    }
}

}

}
