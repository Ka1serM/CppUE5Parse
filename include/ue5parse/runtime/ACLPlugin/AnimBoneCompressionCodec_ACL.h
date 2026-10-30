#pragma once

#include "ue5parse/runtime/ACLPlugin/AnimBoneCompressionCodec_ACLBase.h"

#include "AnimBoneCompressionCodec_ACL.generated.h"

namespace ue5parse::runtime {

// UAnimBoneCompressionCodec_ACL: ACL with the default settings, and a safety fallback codec.
UCLASS()
class UAnimBoneCompressionCodec_ACL : public UAnimBoneCompressionCodec_ACLBase {
    GENERATED_BODY()
public:
    UAnimBoneCompressionCodec* GetCodec(const FString& DDCHandle) override;
    void DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs, const BoneTrackArray& TranslationPairs,
                        const BoneTrackArray& ScalePairs, std::span<FTransform> OutAtoms) const override;
    void DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const override;
};

}
