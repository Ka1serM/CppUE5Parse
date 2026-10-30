#pragma once

#include "ue5parse/runtime/Engine/Animation/AnimBoneCompressionCodec.h"

#include "AnimBoneCompressionCodec_ACLBase.generated.h"

namespace ue5parse::runtime {

// FACLCompressedAnimData: ACL's compressed_tracks, read in place from the compressed byte stream.
struct FACLCompressedAnimData : public ICompressedAnimData {
    std::span<const std::byte> CompressedByteStream;

    void Bind(std::span<const std::byte> BulkData) override { CompressedByteStream = BulkData; }
    bool IsValid() const override;
    // The tracks; null when the stream is not ACL data.
    const void* GetCompressedTracks() const { return IsValid() ? CompressedByteStream.data() : nullptr; }
};

// UAnimBoneCompressionCodec_ACLBase (the ACL plugin's AnimBoneCompressionCodec_ACLBase.h).
UCLASS(Abstract)
class UAnimBoneCompressionCodec_ACLBase : public UAnimBoneCompressionCodec {
    GENERATED_BODY()
public:
    std::unique_ptr<ICompressedAnimData> AllocateAnimData() const override { return std::make_unique<FACLCompressedAnimData>(); }
};

}
