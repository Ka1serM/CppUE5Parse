#pragma once

#include <memory>
#include <span>

#include "CppUE5Parse/Core/AlignedBytes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

namespace ue5parse::runtime {

class UAnimBoneCompressionCodec;
class UAnimBoneCompressionSettings;
class UAnimCurveCompressionCodec;
class UAnimCurveCompressionSettings;

struct FTrackToSkeletonMap {
    int32 BoneTreeIndex = 0;
};

struct BoneTrackPair {
    int32 AtomIndex = 0;
    int32 TrackIndex = 0;
};
using BoneTrackArray = TArray<BoneTrackPair>;

struct FCompressedOffsetData {
    std::span<const int32> OffsetData;
    int32 StripSize = 2;

    bool IsValid() const { return StripSize > 0 && !OffsetData.empty(); }
    int32 GetOffsetData(int32 StripIndex, int32 Offset) const { return OffsetData[StripIndex * StripSize + Offset]; }
};

struct ICompressedAnimData {
    int32 CompressedNumberOfKeys = 0;

    virtual ~ICompressedAnimData() = default;
    virtual void SerializeCompressedData(FArchive& Ar) { Ar << CompressedNumberOfKeys; }
    virtual void Bind(std::span<const std::byte> BulkData) = 0;
    virtual bool IsValid() const = 0;
};

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

struct FAnimCompressedCurveIndexedName {
    FName CurveName;
    int32 CurveIndex = INDEX_NONE;
};

struct FCompressedAnimSequence {
    TArray<FTrackToSkeletonMap> CompressedTrackToSkeletonMapTable;
    TArray<FAnimCompressedCurveIndexedName> IndexedCurveNames;
    AlignedBytes CompressedByteStream;
    TArray<uint8> CompressedCurveByteStream;
    int32 CompressedRawDataSize = 0;
    UAnimBoneCompressionCodec* BoneCompressionCodec = nullptr;
    UAnimCurveCompressionCodec* CurveCompressionCodec = nullptr;
    std::unique_ptr<ICompressedAnimData> CompressedDataStructure;
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
