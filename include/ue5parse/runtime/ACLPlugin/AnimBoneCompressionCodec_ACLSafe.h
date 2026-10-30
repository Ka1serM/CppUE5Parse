#pragma once

#include "ue5parse/runtime/ACLPlugin/AnimBoneCompressionCodec_ACLBase.h"

#include "AnimBoneCompressionCodec_ACLSafe.generated.h"

namespace ue5parse::runtime {

// UAnimBoneCompressionCodec_ACLSafe: ACL with full precision rotations.
UCLASS()
class UAnimBoneCompressionCodec_ACLSafe : public UAnimBoneCompressionCodec_ACLBase {
    GENERATED_BODY()
public:
    void DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs, const BoneTrackArray& TranslationPairs,
                        const BoneTrackArray& ScalePairs, std::span<FTransform> OutAtoms) const override;
    void DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const override;
};

}
