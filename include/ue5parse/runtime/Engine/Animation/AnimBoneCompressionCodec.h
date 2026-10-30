#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimCompressionTypes.h"

#include "AnimBoneCompressionCodec.generated.h"

namespace ue5parse::runtime {

struct FAnimSequenceDecompressionContext;

// UAnimBoneCompressionCodec (UE's Animation/AnimBoneCompressionCodec.h): one way to compress bone tracks. A codec
// allocates its compressed data and decompresses it.
UCLASS(Abstract)
class UAnimBoneCompressionCodec : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FString Description;

    // This codec when its DDC handle matches (UE's GetCodec).
    virtual UAnimBoneCompressionCodec* GetCodec(const FString& DDCHandle);
    // The codec's name, then its outers up to the settings object (UE's GetCodecDDCHandle).
    FString GetCodecDDCHandle() const;
    virtual std::unique_ptr<ICompressedAnimData> AllocateAnimData() const { return nullptr; }
    // Writes the tracks of the pairs into the atoms at the context's time.
    virtual void DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs,
                                const BoneTrackArray& TranslationPairs, const BoneTrackArray& ScalePairs, std::span<FTransform> OutAtoms) const {}
    virtual void DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const {}
};

}
