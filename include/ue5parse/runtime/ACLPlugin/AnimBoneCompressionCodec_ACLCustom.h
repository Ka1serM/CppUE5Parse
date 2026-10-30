#pragma once

#include "ue5parse/runtime/ACLPlugin/AnimBoneCompressionCodec_ACLBase.h"

#include "AnimBoneCompressionCodec_ACLCustom.generated.h"

namespace ue5parse::runtime {

// UAnimBoneCompressionCodec_ACLCustom: ACL with any formats (debug decompression settings).
UCLASS()
class UAnimBoneCompressionCodec_ACLCustom : public UAnimBoneCompressionCodec_ACLBase {
    GENERATED_BODY()
public:
    void DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs, const BoneTrackArray& TranslationPairs,
                        const BoneTrackArray& ScalePairs, std::span<FTransform> OutAtoms) const override;
    void DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const override;
};

}
