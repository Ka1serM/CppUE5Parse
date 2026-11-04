#pragma once

#include "Engine/Plugins/Animation/ACLPlugin/Source/ACLPlugin/AnimBoneCompressionCodec_ACLBase.h"

#include "AnimBoneCompressionCodec_ACLDatabase.generated.h"

namespace ue5parse::runtime {

struct FACLDatabaseCompressedAnimData : public FACLCompressedAnimData {
    uint32 SequenceNameHash = 0;

    void SerializeCompressedData(FArchive& Ar) override {
        FACLCompressedAnimData::SerializeCompressedData(Ar);
        Ar << SequenceNameHash;
    }
};

UCLASS()
class UAnimBoneCompressionCodec_ACLDatabase : public UAnimBoneCompressionCodec_ACLBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UObject> DatabaseAsset = nullptr;

    std::unique_ptr<ICompressedAnimData> AllocateAnimData() const override { return std::make_unique<FACLDatabaseCompressedAnimData>(); }
    void DecompressPose(FAnimSequenceDecompressionContext& DecompContext, const BoneTrackArray& RotationPairs, const BoneTrackArray& TranslationPairs,
                        const BoneTrackArray& ScalePairs, std::span<FTransform> OutAtoms) const override;
    void DecompressBone(FAnimSequenceDecompressionContext& DecompContext, int32 TrackIndex, FTransform& OutAtom) const override;
};

}
