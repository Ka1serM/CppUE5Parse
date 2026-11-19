#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimBoneCompressionCodec.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimEnums.h"

#include "AnimCompress.generated.h"

namespace ue5parse::runtime {

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
