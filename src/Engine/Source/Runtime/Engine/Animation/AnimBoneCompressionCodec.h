#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimCompressionTypes.h"

#include "AnimBoneCompressionCodec.generated.h"

namespace ue5parse::runtime {

struct FAnimSequenceDecompressionContext;

UCLASS(Abstract)
class UAnimBoneCompressionCodec : public UObject {
    GENERATED_BODY()
public:
    UPROPERTY()
    FString Description;

    virtual UAnimBoneCompressionCodec* GetCodec(const FString& DDCHandle);
    FString GetCodecDDCHandle() const;
    virtual std::unique_ptr<ICompressedAnimData> AllocateAnimData() const { return nullptr; }
    virtual void DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs, const BoneTrackArray& TranslationPairs,
                                const BoneTrackArray& ScalePairs, std::span<FTransform> OutAtoms) const {}
    virtual void DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const {}
};

}
