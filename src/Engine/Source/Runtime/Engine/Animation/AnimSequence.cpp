#include "Engine/Source/Runtime/Engine/Animation/AnimSequence.h"

#include <cmath>

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimBoneCompressionSettings.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimCompress.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimCurveCompressionSettings.h"
#include "Engine/Source/Runtime/Engine/Animation/Skeleton.h"
#include "Engine/Source/Runtime/Engine/Animation/SkeletonRemapping.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

void UAnimSequence::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    FStripDataFlags StripFlags(Ar);
    if (!StripFlags.IsEditorDataStripped()) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "UAnimSequence.EditorData");
        return;
    }
    if (!Ar.CustomVerAtLeast(FFrameworkObjectVersion::GUID, FFrameworkObjectVersion::MoveCompressedAnimDataToTheDDC, EGame::GAME_UE4_12)) {
        CompressedData.CompressedTrackToSkeletonMapTable = TrackToSkeletonMapTable;
        CompressedCurveData = RawCurveData;
        SerializeCompressedByteStreamUE4(Ar, NumFrames);
        return;
    }
    bool bSerializeCompressedData = false;
    Ar << bSerializeCompressedData;
    if (!bSerializeCompressedData)
        return;
    if (Ar.GetGame() < EGame::GAME_UE4_23)
        SerializeCompressedDataUE4(Ar);
    else if (Ar.GetGame() < EGame::GAME_UE4_25)
        CompressedData.SerializeCompressedDataUE4(Ar, CurveCompressionSettings);
    else
        CompressedData.SerializeCompressedData(Ar, BoneCompressionSettings, CurveCompressionSettings);
    if (!Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::AnimSequenceRawDataOnlyFlagRemoval, EGame::GAME_UE5_6)) {
        bool bUseRawDataOnly = false;
        Ar << bUseRawDataOnly;
    }
}

void UAnimSequence::SerializeCompressedDataUE4(FArchive& Ar) {
    Ar << KeyEncodingFormat << TranslationCompressionFormat << RotationCompressionFormat << ScaleCompressionFormat;
    Ar << CompressedTrackOffsets << CompressedScaleOffsets.OffsetData << CompressedScaleOffsets.StripSize;
    if (Ar.GetGame() >= EGame::GAME_UE4_21) {
        const int32 NumSegments = Ar.ReadCount(15);
        //! Segments split the byte stream for ACL, which these UE 4 versions only ship as a plugin.
        if (NumSegments != 0) {
            Ar.SetError(ErrorCode::UnsupportedVersion, "UAnimSequence.CompressedSegments");
            return;
        }
    }
    CompressedData.SerializeTrackToSkeletonMapTable(Ar);
    if (Ar.GetGame() >= EGame::GAME_UE4_22) {
        CompressedData.SerializeCurveNames(Ar);
    } else {
        FRawCurveTracks::StaticStruct()->SerializeTaggedProperties(Ar, reinterpret_cast<uint8*>(&CompressedCurveData));
        CompressedCurveData.PostSerialize();
        if (Ar.GetGame() < EGame::GAME_UE4_13) {
            for (int32 CurveIndex = 0; CurveIndex < CompressedCurveData.FloatCurves.Num(); ++CurveIndex) {
                uint16 CurveUid = 0;
                Ar << CurveUid;
            }
        }
    }
    if (Ar.GetGame() >= EGame::GAME_UE4_17)
        Ar << CompressedData.CompressedRawDataSize;
    int32 NumberOfKeys = NumFrames;
    if (Ar.GetGame() >= EGame::GAME_UE4_22)
        Ar << NumberOfKeys;
    SerializeCompressedByteStreamUE4(Ar, NumberOfKeys);
    if (Ar.GetGame() >= EGame::GAME_UE4_22) {
        CompressedData.SerializeCurveData(Ar, CurveCompressionSettings);
        CompressedData.RebuildCurveIndexTable();
    }
}

void UAnimSequence::SerializeCompressedByteStreamUE4(FArchive& Ar, int32 NumberOfKeys) {
    const int32 NumBytes = Ar.ReadCount(1);
    std::span<const std::byte> ByteStream = Ar.ReadView(NumBytes);
    if (Ar.IsError())
        return;
    std::unique_ptr<FUECompressedAnimData> AnimData = std::make_unique<FUECompressedAnimData>();
    AnimData->CompressedNumberOfKeys = NumberOfKeys;
    AnimData->KeyEncodingFormat = KeyEncodingFormat;
    AnimData->TranslationCompressionFormat = TranslationCompressionFormat;
    AnimData->RotationCompressionFormat = RotationCompressionFormat;
    AnimData->ScaleCompressionFormat = ScaleCompressionFormat;
    AnimData->CompressedScaleOffsets.StripSize = CompressedScaleOffsets.StripSize;
    AnimData->BindArrays(std::span<const int32>(CompressedTrackOffsets.GetData(), CompressedTrackOffsets.Num()),
                         std::span<const int32>(CompressedScaleOffsets.OffsetData.GetData(), CompressedScaleOffsets.OffsetData.Num()), ByteStream,
                         CompressedData.CompressedByteStream);
    CompressedData.CompressedDataStructure = std::move(AnimData);
    //! UE 4 picks the decoder from the key and component formats, so the codec object holds no settings.
    CompressedData.BoneCompressionCodec = Cast<UAnimCompress>(UAnimCompress::StaticClass()->GetDefaultObject());
}

bool UAnimSequence::IsValidAdditive() const {
    if (AdditiveAnimType == AAT_None)
        return false;
    switch (RefPoseType) {
    case ABPT_RefPose:
        return true;
    case ABPT_AnimScaled:
        return RefPoseSeq != nullptr;
    case ABPT_AnimFrame:
        return RefPoseSeq != nullptr && RefFrameIndex >= 0;
    case ABPT_LocalAnimFrame:
        return RefFrameIndex >= 0;
    default:
        return false;
    }
}

const TArray<FTransform>& UAnimSequence::GetRetargetTransforms() const {
    if (RetargetSource.isNone() && RetargetSourceAssetReferencePose.Num() > 0)
        return RetargetSourceAssetReferencePose;
    static const TArray<FTransform> EmptyTransformArray;
    return Skeleton ? Skeleton->GetRefLocalPoses(RetargetSource) : EmptyTransformArray;
}

FName UAnimSequence::GetRetargetTransformsSourceName() const {
    if (RetargetSource.isNone() && RetargetSourceAssetReferencePose.Num() > 0)
        return GetOutermost()->GetFName();
    return RetargetSource;
}

void UAnimSequence::GetAnimationPose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const {
    GetBonePose(OutAnimationPoseData, ExtractionContext);
}

namespace {

int32 NumberOfFramesOf(const FFrameRate& Rate, double Length) {
    return static_cast<int32>(std::floor(Length * Rate.Numerator / Rate.Denominator + 0.5));
}

}

void UAnimSequence::GetBonePose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const {
    FCompactPose& OutPose = OutAnimationPoseData.GetPose();
    const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
    const bool bIsBakedAdditive = IsValidAdditive();
    if (Skeleton == nullptr) {
        if (bIsBakedAdditive)
            OutPose.ResetToAdditiveIdentity();
        else
            OutPose.ResetToRefPose();
        return;
    }
    if (bIsBakedAdditive) {
        OutPose.ResetToAdditiveIdentity();
    } else if (RequiredBones.GetDisableRetargeting()) {
        const TArray<FTransform>& AuthoredOnRefSkeleton = GetRetargetTransforms();
        const FSkeletonRemapping& SkeletonRemapping = UE::Anim::FSkeletonRemappingRegistry::Get().GetRemapping(Skeleton, RequiredBones.GetSkeletonAsset());
        for (FCompactPoseBoneIndex PoseBoneIndex(0); PoseBoneIndex < OutPose.GetNumBones(); ++PoseBoneIndex) {
            int32 SkeletonBoneIndex = RequiredBones.GetSkeletonIndex(PoseBoneIndex);
            if (SkeletonRemapping.IsValid())
                SkeletonBoneIndex = SkeletonRemapping.GetSourceSkeletonBoneIndex(SkeletonBoneIndex);
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

void UAnimSequence::GetAdditiveBasePose(FAnimationPoseData& OutAnimationPoseData, const FAnimExtractContext& ExtractionContext) const {
    FAnimExtractContext BasePoseExtractionContext(ExtractionContext);
    switch (RefPoseType) {
    case ABPT_AnimScaled: {
        const float Fraction = GetPlayLength() > 0.f ? FMath::Clamp(static_cast<float>(ExtractionContext.CurrentTime) / GetPlayLength(), 0.f, 1.f) : 0.f;
        BasePoseExtractionContext.CurrentTime = RefPoseSeq->GetPlayLength() * Fraction;
        RefPoseSeq->GetBonePose(OutAnimationPoseData, BasePoseExtractionContext);
        return;
    }
    case ABPT_AnimFrame: {
        const int32 NumKeys = RefPoseSeq->GetNumberOfSampledKeys();
        const float Fraction = NumKeys > 0 ? FMath::Clamp(static_cast<float>(RefFrameIndex) / static_cast<float>(NumKeys), 0.f, 1.f) : 0.f;
        BasePoseExtractionContext.CurrentTime = RefPoseSeq->GetPlayLength() * Fraction;
        RefPoseSeq->GetBonePose(OutAnimationPoseData, BasePoseExtractionContext);
        return;
    }
    case ABPT_LocalAnimFrame: {
        const int32 NumKeys = GetNumberOfSampledKeys();
        const float Fraction = NumKeys > 0 ? FMath::Clamp(static_cast<float>(RefFrameIndex) / static_cast<float>(NumKeys), 0.f, 1.f) : 0.f;
        BasePoseExtractionContext.CurrentTime = GetPlayLength() * Fraction;
        GetBonePose(OutAnimationPoseData, BasePoseExtractionContext);
        return;
    }
    default:
        OutAnimationPoseData.GetPose().ResetToRefPose();
        return;
    }
}

void UAnimSequence::EvaluateCurveData(FBlendedCurve& OutCurve, float CurrentTime) const {
    if (CompressedData.CurveCompressionCodec != nullptr) {
        CompressedData.CurveCompressionCodec->DecompressCurves(CompressedData, OutCurve, CurrentTime);
        return;
    }
    for (const FFloatCurve& Curve : CompressedCurveData.FloatCurves)
        OutCurve.Set(Curve.GetName(), Curve.Evaluate(CurrentTime));
}

float UAnimSequence::EvaluateCurveData(FName CurveName, float CurrentTime) const {
    if (CompressedData.CurveCompressionCodec != nullptr)
        return CompressedData.CurveCompressionCodec->DecompressCurve(CompressedData, CurveName, CurrentTime);
    for (const FFloatCurve& Curve : CompressedCurveData.FloatCurves)
        if (Curve.GetName() == CurveName)
            return Curve.Evaluate(CurrentTime);
    return 0.0f;
}

FTransform UAnimSequence::ExtractRootTrackTransform(float Time, const FBoneContainer* RequiredBones) const {
    const TArray<FTrackToSkeletonMap>& TrackToSkeletonMap = CompressedData.CompressedTrackToSkeletonMapTable;
    if (TrackToSkeletonMap.Num() > 0 && TrackToSkeletonMap[0].BoneTreeIndex == 0 && CompressedData.IsValid() && Skeleton != nullptr) {
        const FFrameRate& Rate = PlatformTargetFrameRate.Default;
        FAnimSequenceDecompressionContext DecompContext(Rate, NumberOfFramesOf(Rate, GetPlayLength()), Interpolation, GetRetargetTransformsSourceName(),
                                                        CompressedData, Skeleton->GetRefLocalPoses(), Skeleton, IsValidAdditive());
        DecompContext.Seek(Time);
        FTransform RootTransform;
        CompressedData.BoneCompressionCodec->DecompressBone(DecompContext, 0, RootTransform);
        return RootTransform;
    }
    if (RequiredBones != nullptr && RequiredBones->GetReferenceSkeleton().GetNum() > 0)
        return RequiredBones->GetReferenceSkeleton().GetRefBonePose()[0];
    if (RequiredBones == nullptr && Skeleton != nullptr && Skeleton->GetReferenceSkeleton().GetNum() > 0)
        return Skeleton->GetReferenceSkeleton().GetRefBonePose()[0];
    return FTransform::Identity;
}

void UAnimSequence::GetBoneTransform(FTransform& OutAtom, const FSkeletonPoseBoneIndex BoneIndex, const double Time) const {
    if (!CompressedData.IsValid() || Skeleton == nullptr)
        return;
    const TArray<FTrackToSkeletonMap>& TrackToSkeletonMap = CompressedData.CompressedTrackToSkeletonMapTable;
    for (int32 TrackIndex = 0; TrackIndex < TrackToSkeletonMap.Num(); ++TrackIndex) {
        if (TrackToSkeletonMap[TrackIndex].BoneTreeIndex != BoneIndex.GetInt())
            continue;
        const FFrameRate& Rate = PlatformTargetFrameRate.Default;
        FAnimSequenceDecompressionContext DecompContext(Rate, NumberOfFramesOf(Rate, GetPlayLength()), Interpolation, GetRetargetTransformsSourceName(),
                                                        CompressedData, Skeleton->GetRefLocalPoses(), Skeleton, IsValidAdditive());
        DecompContext.Seek(Time);
        CompressedData.BoneCompressionCodec->DecompressBone(DecompContext, TrackIndex, OutAtom);
        return;
    }
}

}
