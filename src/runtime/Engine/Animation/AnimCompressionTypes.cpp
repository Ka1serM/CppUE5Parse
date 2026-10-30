#include "ue5parse/runtime/Engine/Animation/AnimCompressionTypes.h"

#include <algorithm>

#include "ue5parse/runtime/CoreUObject/Serialization/BulkData.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"
#include "ue5parse/runtime/Engine/Animation/AnimBoneCompressionSettings.h"
#include "ue5parse/runtime/Engine/Animation/AnimCurveCompressionSettings.h"

// UE's AnimCompressionTypes.cpp (UE 5.3): FCompressedAnimSequence and FUECompressedAnimData on load.

namespace ue5parse::runtime {

void FUECompressedAnimData::SerializeCompressedData(FArchive& Ar) {
    ICompressedAnimData::SerializeCompressedData(Ar);
    Ar << KeyEncodingFormat << TranslationCompressionFormat << RotationCompressionFormat << ScaleCompressionFormat;
    // The view sizes; Bind points the views into the byte stream.
    Ar << NumBytes << NumTrackOffsets << NumScaleOffsets << CompressedScaleOffsets.StripSize;
}

void FUECompressedAnimData::Bind(std::span<const std::byte> BulkData) {
    // InitViewsFromBuffer: the track offsets, the scale offsets, then the keys.
    const uint64 Needed = uint64(NumTrackOffsets) * 4 + uint64(NumScaleOffsets) * 4 + uint64(NumBytes);
    if (NumTrackOffsets < 0 || NumScaleOffsets < 0 || NumBytes < 0 || Needed > BulkData.size()) {
        CompressedTrackOffsets = {};
        CompressedScaleOffsets.OffsetData = {};
        CompressedByteStream = {};
        return;
    }
    const std::byte* Data = BulkData.data();
    CompressedTrackOffsets = std::span<const int32>(reinterpret_cast<const int32*>(Data), static_cast<std::size_t>(NumTrackOffsets));
    Data += std::size_t(NumTrackOffsets) * 4;
    CompressedScaleOffsets.OffsetData = std::span<const int32>(reinterpret_cast<const int32*>(Data), static_cast<std::size_t>(NumScaleOffsets));
    Data += std::size_t(NumScaleOffsets) * 4;
    CompressedByteStream = std::span<const uint8>(reinterpret_cast<const uint8*>(Data), static_cast<std::size_t>(NumBytes));
}

void FCompressedAnimSequence::SerializeCompressedData(FArchive& Ar, UAnimBoneCompressionSettings* BoneCompressionSettings,
                                                      UAnimCurveCompressionSettings* CurveCompressionSettings) {
    Ar << CompressedRawDataSize;
    const int32 TrackCount = Ar.ReadCount(4);
    CompressedTrackToSkeletonMapTable.SetNum(TrackCount);
    for (int32 i = 0; i < TrackCount; i++) Ar << CompressedTrackToSkeletonMapTable[i].BoneTreeIndex;
    const int32 CurveCount = Ar.ReadCount(8);
    IndexedCurveNames.SetNum(CurveCount);
    for (int32 i = 0; i < CurveCount; i++) Ar << IndexedCurveNames[i].CurveName;

    int32 NumBytes = 0;
    bool bUseBulkDataForLoad = false;
    Ar << NumBytes << bUseBulkDataForLoad;
    if (Ar.IsError()) return;
    if (NumBytes < 0) {
        Ar.SetError(ErrorCode::InvalidData, "FCompressedAnimSequence.NumBytes");
        return;
    }
    if (bUseBulkDataForLoad) {
        FByteBulkData OptionalBulk;
        OptionalBulk.Serialize(Ar);
        CompressedByteStream.resize(OptionalBulk.GetPayloadSize());
        Result<void> Read = OptionalBulk.ReadInto(CompressedByteStream);
        if (!Read) {
            Ar.SetError(Read.error().code, "FCompressedAnimSequence.BulkData");
            return;
        }
    } else {
        std::span<const std::byte> Bytes = Ar.ReadView(NumBytes);
        CompressedByteStream.assign(Bytes.begin(), Bytes.end());
    }
    Ar << BoneCodecDDCHandle << CurveCodecPath;
    const int32 NumCurveBytes = Ar.ReadCount(1);
    CompressedCurveByteStream.SetNum(NumCurveBytes);
    if (NumCurveBytes > 0) Ar.Serialize(CompressedCurveByteStream.GetData(), NumCurveBytes);
    if (Ar.IsError()) return;

    BoneCompressionCodec = BoneCompressionSettings ? BoneCompressionSettings->GetCodec(BoneCodecDDCHandle) : nullptr;
    CurveCompressionCodec = CurveCompressionSettings ? CurveCompressionSettings->GetCodec(CurveCodecPath) : nullptr;
    if (BoneCompressionCodec != nullptr) CompressedDataStructure = BoneCompressionCodec->AllocateAnimData();
    if (CompressedDataStructure != nullptr) {
        CompressedDataStructure->SerializeCompressedData(Ar);
        CompressedDataStructure->Bind(std::span<const std::byte>(CompressedByteStream.data(), CompressedByteStream.size()));
    } else if (!CompressedByteStream.empty()) {
        // Without the codec the size of its data is unknown: the rest of the export is skipped.
        ReportLoadProblem("anim.bone_codec_missing", BoneCodecDDCHandle);
        Ar.SkipBytes(Ar.GetRemaining());
    }
    RebuildCurveIndexTable();
}

void FCompressedAnimSequence::RebuildCurveIndexTable() {
    TArray<int32> IndexArray;
    IndexArray.SetNum(IndexedCurveNames.Num());
    for (int32 NameIndex = 0; NameIndex < IndexArray.Num(); ++NameIndex) IndexArray[NameIndex] = NameIndex;
    std::sort(IndexArray.begin(), IndexArray.end(), [this](int32 LHS, int32 RHS) {
        const FName& A = IndexedCurveNames[LHS].CurveName;
        const FName& B = IndexedCurveNames[RHS].CurveName;
        return A.getHash() != B.getHash() ? A.getHash() < B.getHash() : A.getNumber() < B.getNumber();
    });
    for (int32 NameIndex = 0; NameIndex < IndexedCurveNames.Num(); ++NameIndex) IndexedCurveNames[NameIndex].CurveIndex = IndexArray[NameIndex];
}

}
