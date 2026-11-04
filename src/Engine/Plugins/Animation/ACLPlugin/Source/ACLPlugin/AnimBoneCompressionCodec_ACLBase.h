#pragma once

#include "Engine/Source/Runtime/Engine/Animation/AnimBoneCompressionCodec.h"

#include "AnimBoneCompressionCodec_ACLBase.generated.h"

namespace ue5parse::runtime {

struct FACLCompressedAnimData : public ICompressedAnimData {
    std::span<const std::byte> CompressedByteStream;

    void Bind(std::span<const std::byte> BulkData) override { CompressedByteStream = BulkData; }
    bool IsValid() const override;
    const void* GetCompressedTracks() const { return IsValid() ? CompressedByteStream.data() : nullptr; }
};

UCLASS(Abstract)
class UAnimBoneCompressionCodec_ACLBase : public UAnimBoneCompressionCodec {
    GENERATED_BODY()
public:
    std::unique_ptr<ICompressedAnimData> AllocateAnimData() const override { return std::make_unique<FACLCompressedAnimData>(); }
};

}
