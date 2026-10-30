#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimBoneCompressionCodec.h"

#include "AnimCompress.generated.h"

namespace ue5parse::runtime {

enum AnimationCompressionFormat : int32 {
    ACF_None,
    ACF_Float96NoW,
    ACF_Fixed48NoW,
    ACF_IntervalFixed32NoW,
    ACF_Fixed32NoW,
    ACF_Float32NoW,
    ACF_Identity,
};

enum AnimationKeyFormat : int32 { AKF_ConstantKeyLerp, AKF_VariableKeyLerp, AKF_PerTrackCompression };

// UAnimCompress (UE's Animation/AnimCompress.h): the base of the engine's own key reduction codecs.
UCLASS(Abstract)
class UAnimCompress : public UAnimBoneCompressionCodec {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bNeedsSkeleton = false;
    UPROPERTY()
    TEnumAsByte<AnimationCompressionFormat> TranslationCompressionFormat = ACF_None;
    UPROPERTY()
    TEnumAsByte<AnimationCompressionFormat> RotationCompressionFormat = ACF_Float96NoW;
    UPROPERTY()
    TEnumAsByte<AnimationCompressionFormat> ScaleCompressionFormat = ACF_None;

    std::unique_ptr<ICompressedAnimData> AllocateAnimData() const override { return std::make_unique<FUECompressedAnimData>(); }
    void DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs, const BoneTrackArray& TranslationPairs,
                        const BoneTrackArray& ScalePairs, std::span<FTransform> OutAtoms) const override;
    void DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const override;
};

}
