#pragma once

#include <memory>
#include <span>

#include "ue5parse/core/AlignedBytes.h"
#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

namespace ue5parse::runtime {

class UAnimBoneCompressionCodec;
class UAnimBoneCompressionSettings;
class UAnimCurveCompressionCodec;
class UAnimCurveCompressionSettings;

// FTrackToSkeletonMap: the skeleton bone of a compressed track.
struct FTrackToSkeletonMap {
    int32 BoneTreeIndex = 0;
};

// BoneTrackPair: a pose bone (atom) and the compressed track that animates it (UE's AnimEncoding.h).
struct BoneTrackPair {
    int32 AtomIndex = 0;
    int32 TrackIndex = 0;
};
using BoneTrackArray = TArray<BoneTrackPair>;

// FCompressedOffsetData: per-track offsets into the key stream, StripSize entries per track.
struct FCompressedOffsetData {
    std::span<const int32> OffsetData;
    int32 StripSize = 2;

    bool IsValid() const { return StripSize > 0 && !OffsetData.empty(); }
    int32 GetOffsetData(int32 StripIndex, int32 Offset) const { return OffsetData[StripIndex * StripSize + Offset]; }
};

// ICompressedAnimData: the part of the compressed data that belongs to a bone codec. The codec allocates it and reads
// its header; Bind points it into the compressed byte stream.
struct ICompressedAnimData {
    int32 CompressedNumberOfKeys = 0;

    virtual ~ICompressedAnimData() = default;
    virtual void SerializeCompressedData(FArchive& Ar) { Ar << CompressedNumberOfKeys; }
    virtual void Bind(std::span<const std::byte> BulkData) = 0;
    virtual bool IsValid() const = 0;
};

// FUECompressedAnimData: the data of the engine's own codecs (UAnimCompress): the formats, then views of the track
// offsets, the scale offsets and the keys.
struct FUECompressedAnimData : public ICompressedAnimData {
    uint8 KeyEncodingFormat = 0;
    uint8 TranslationCompressionFormat = 0;
    uint8 RotationCompressionFormat = 0;
    uint8 ScaleCompressionFormat = 0;
    std::span<const int32> CompressedTrackOffsets;
    FCompressedOffsetData CompressedScaleOffsets;
    std::span<const uint8> CompressedByteStream;

    void SerializeCompressedData(FArchive& Ar) override;
    void Bind(std::span<const std::byte> BulkData) override;
    bool IsValid() const override { return !CompressedByteStream.empty() || CompressedTrackOffsets.empty(); }

private:
    int32 NumTrackOffsets = 0;
    int32 NumScaleOffsets = 0;
    int32 NumBytes = 0;
};

// FAnimCompressedCurveIndexedName: a compressed curve's name and, after loading, the index of the curves in name order.
struct FAnimCompressedCurveIndexedName {
    FName CurveName;
    int32 CurveIndex = INDEX_NONE;
};

// FCompressedAnimSequence (UE's Animation/AnimCompressionTypes.h): the cooked bone and curve data of a sequence and the
// codecs that read it.
struct FCompressedAnimSequence {
    TArray<FTrackToSkeletonMap> CompressedTrackToSkeletonMapTable;
    TArray<FAnimCompressedCurveIndexedName> IndexedCurveNames;
    // The bone data, 16-byte aligned as ACL reads it in place.
    AlignedBytes CompressedByteStream;
    TArray<uint8> CompressedCurveByteStream;
    int32 CompressedRawDataSize = 0;
    UAnimBoneCompressionCodec* BoneCompressionCodec = nullptr;
    UAnimCurveCompressionCodec* CurveCompressionCodec = nullptr;
    std::unique_ptr<ICompressedAnimData> CompressedDataStructure;
    // The codec names the data was made with; a codec that is not found leaves CompressedDataStructure null.
    FString BoneCodecDDCHandle;
    FString CurveCodecPath;

    void SerializeCompressedData(FArchive& Ar, UAnimBoneCompressionSettings* BoneCompressionSettings,
                                 UAnimCurveCompressionSettings* CurveCompressionSettings);
    int32 GetSkeletonIndexFromTrackIndex(int32 TrackIndex) const { return CompressedTrackToSkeletonMapTable[TrackIndex].BoneTreeIndex; }
    bool IsValid() const { return CompressedDataStructure != nullptr && BoneCompressionCodec != nullptr; }

private:
    void RebuildCurveIndexTable();
};

}
