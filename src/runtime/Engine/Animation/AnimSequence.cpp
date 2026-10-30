#include "ue5parse/runtime/Engine/Animation/AnimSequence.h"

#include <cmath>

#include "ue5parse/runtime/Engine/Animation/AnimBoneCompressionSettings.h"
#include "ue5parse/runtime/Engine/Animation/AnimCurveCompressionSettings.h"
#include "ue5parse/runtime/Engine/Animation/Skeleton.h"
#include "ue5parse/runtime/Engine/Animation/SkeletonRemapping.h"
#include "ue5parse/runtime/Engine/EngineUtils.h"

// UE's AnimSequence.cpp (UE 5.3): cooked data only, so poses always come from the compressed data.

namespace ue5parse::runtime {

void UAnimSequence::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError()) return;
    FStripDataFlags StripFlags(Ar);
    if (!StripFlags.IsEditorDataStripped()) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "UAnimSequence.EditorData");
        return;
    }
    bool bSerializeCompressedData = false;
    Ar << bSerializeCompressedData;
    if (!bSerializeCompressedData) return;
    CompressedData.SerializeCompressedData(Ar, BoneCompressionSettings, CurveCompressionSettings);
    // bUseRawDataOnly, written after the compressed data.
    if (!Ar.IsError() && Ar.GetRemaining() >= 4) Ar.SkipBytes(4);
}

bool UAnimSequence::IsValidAdditive() const {
    if (AdditiveAnimType == AAT_None) return false;
    switch (RefPoseType) {
    case ABPT_RefPose: return true;
    case ABPT_AnimScaled: return RefPoseSeq != nullptr;
    case ABPT_AnimFrame: return RefPoseSeq != nullptr && RefFrameIndex >= 0;
    case ABPT_LocalAnimFrame: return RefFrameIndex >= 0;
    default: return false;
    }
}

const TArray<FTransform>& UAnimSequence::GetRetargetTransforms() const {
    if (RetargetSource.isNone() && RetargetSourceAssetReferencePose.Num() > 0) return RetargetSourceAssetReferencePose;
    static const TArray<FTransform> EmptyTransformArray;
    return Skeleton ? Skeleton->GetRefLocalPoses(RetargetSource) : EmptyTransformArray;
}

FName UAnimSequence::GetRetargetTransformsSourceName() const {
    if (RetargetSource.isNone() && RetargetSourceAssetReferencePose.Num() > 0) return GetOutermost()->GetFName();
    return RetargetSource;
}

void UAnimSequence::GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const {
    GetBonePose(OutAnimationPoseData, ExtractionContext);
}

namespace {

int32 NumberOfFramesOf(const FFrameRate& Rate, double Length) {
    // FFrameRate::AsFrameTime(Length).RoundToFrame().
    return static_cast<int32>(std::floor(Length * Rate.Numerator / Rate.Denominator + 0.5));
}

}

void UAnimSequence::GetBonePose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const {
    FCompactPose& OutPose = OutAnimationPoseData.GetPose();
    const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
    const bool bIsBakedAdditive = IsValidAdditive();
    if (Skeleton == nullptr) {
        if (bIsBakedAdditive) OutPose.ResetToAdditiveIdentity();
        else OutPose.ResetToRefPose();
        return;
    }
    if (bIsBakedAdditive) {
        OutPose.ResetToAdditiveIdentity();
    } else if (RequiredBones.GetDisableRetargeting()) {
        const TArray<FTransform>& AuthoredOnRefSkeleton = GetRetargetTransforms();
        const FSkeletonRemapping& SkeletonRemapping = UE::Anim::FSkeletonRemappingRegistry::Get().GetRemapping(Skeleton, RequiredBones.GetSkeletonAsset());
        for (FCompactPoseBoneIndex PoseBoneIndex(0); PoseBoneIndex < OutPose.GetNumBones(); ++PoseBoneIndex) {
            int32 SkeletonBoneIndex = RequiredBones.GetSkeletonIndex(PoseBoneIndex);
            if (SkeletonRemapping.IsValid()) SkeletonBoneIndex = SkeletonRemapping.GetSourceSkeletonBoneIndex(SkeletonBoneIndex);
            if (SkeletonBoneIndex != INDEX_NONE && AuthoredOnRefSkeleton.IsValidIndex(SkeletonBoneIndex))
                OutPose[PoseBoneIndex] = AuthoredOnRefSkeleton[SkeletonBoneIndex];
        }
    } else {
        OutPose.ResetToRefPose();
    }

    const int32 NumTracks = CompressedData.CompressedTrackToSkeletonMapTable.Num();
    if (NumTracks != 0 && CompressedData.IsValid()) {
        const FRootMotionReset RootMotionReset(bEnableRootMotion, RootMotionRootLock.GetValue(), !ExtractionContext.bIgnoreRootLock && bForceRootLock,
                                               ExtractRootTrackTransform(0.0f, &RequiredBones), bIsBakedAdditive);
        const FFrameRate& Rate = PlatformTargetFrameRate.Default;
        FAnimSequenceDecompressionContext DecompContext(Rate, NumberOfFramesOf(Rate, GetPlayLength()), Interpolation, GetRetargetTransformsSourceName(),
                                                        CompressedData, Skeleton->GetRefLocalPoses(), Skeleton, bIsBakedAdditive);
        UE::Anim::Decompression::DecompressPose(OutPose, CompressedData, ExtractionContext, DecompContext, GetRetargetTransforms(), RootMotionReset);
    }
    EvaluateCurveData(OutAnimationPoseData.GetCurve(), static_cast<float>(ExtractionContext.CurrentTime));
}

void UAnimSequence::EvaluateCurveData(FBlendedCurve& OutCurve, float CurrentTime) const {
    if (CompressedData.CurveCompressionCodec != nullptr) CompressedData.CurveCompressionCodec->DecompressCurves(CompressedData, OutCurve, CurrentTime);
}

float UAnimSequence::EvaluateCurveData(FName CurveName, float CurrentTime) const {
    return CompressedData.CurveCompressionCodec ? CompressedData.CurveCompressionCodec->DecompressCurve(CompressedData, CurveName, CurrentTime) : 0.0f;
}

FTransform UAnimSequence::ExtractRootTrackTransform(float Time, const FBoneContainer* RequiredBones) const {
    const TArray<FTrackToSkeletonMap>& TrackToSkeletonMap = CompressedData.CompressedTrackToSkeletonMapTable;
    if (TrackToSkeletonMap.Num() > 0 && TrackToSkeletonMap[0].BoneTreeIndex == 0 && CompressedData.IsValid() && Skeleton != nullptr) {
        // GetBoneTransform of the root track.
        const FFrameRate& Rate = PlatformTargetFrameRate.Default;
        FAnimSequenceDecompressionContext DecompContext(Rate, NumberOfFramesOf(Rate, GetPlayLength()), Interpolation, GetRetargetTransformsSourceName(),
                                                        CompressedData, Skeleton->GetRefLocalPoses(), Skeleton, IsValidAdditive());
        DecompContext.Seek(Time);
        FTransform RootTransform;
        CompressedData.BoneCompressionCodec->DecompressBone(DecompContext, 0, RootTransform);
        return RootTransform;
    }
    if (RequiredBones != nullptr && RequiredBones->GetReferenceSkeleton().GetNum() > 0) return RequiredBones->GetReferenceSkeleton().GetRefBonePose()[0];
    if (RequiredBones == nullptr && Skeleton != nullptr && Skeleton->GetReferenceSkeleton().GetNum() > 0)
        return Skeleton->GetReferenceSkeleton().GetRefBonePose()[0];
    return FTransform::Identity;
}

}
