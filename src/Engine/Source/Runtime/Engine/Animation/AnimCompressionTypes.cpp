#include "Engine/Source/Runtime/Engine/Animation/AnimCompressionTypes.h"

#include <algorithm>

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimBoneCompressionSettings.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimCompress.h"
#include "Engine/Source/Runtime/Engine/Animation/AnimCurveCompressionSettings.h"
#include "Engine/Source/Runtime/Engine/Animation/SmartName.h"

namespace ue5parse::runtime {

void FUECompressedAnimData::SerializeCompressedData(FArchive& Ar) {
    ICompressedAnimData::SerializeCompressedData(Ar);
    Ar << KeyEncodingFormat << TranslationCompressionFormat << RotationCompressionFormat << ScaleCompressionFormat;
    Ar << NumBytes << NumTrackOffsets << NumScaleOffsets << CompressedScaleOffsets.StripSize;
}

void FUECompressedAnimData::SerializeCompressedDataUE4(FArchive& Ar) {
    Ar << KeyEncodingFormat << TranslationCompressionFormat << RotationCompressionFormat << ScaleCompressionFormat;
    Ar << CompressedNumberOfKeys << NumTrackOffsets << NumScaleOffsets << CompressedScaleOffsets.StripSize << NumBytes;
}

void FUECompressedAnimData::BindArrays(std::span<const int32> TrackOffsets, std::span<const int32> ScaleOffsets, std::span<const std::byte> ByteStream,
                                       AlignedBytes& OutBuffer) {
    NumTrackOffsets = static_cast<int32>(TrackOffsets.size());
    NumScaleOffsets = static_cast<int32>(ScaleOffsets.size());
    NumBytes = static_cast<int32>(ByteStream.size());
    OutBuffer.resize(TrackOffsets.size_bytes() + ScaleOffsets.size_bytes() + ByteStream.size());
    std::byte* Data = OutBuffer.data();
    Data = std::copy_n(reinterpret_cast<const std::byte*>(TrackOffsets.data()), TrackOffsets.size_bytes(), Data);
    Data = std::copy_n(reinterpret_cast<const std::byte*>(ScaleOffsets.data()), ScaleOffsets.size_bytes(), Data);
    std::copy(ByteStream.begin(), ByteStream.end(), Data);
    Bind(std::span<const std::byte>(OutBuffer.data(), OutBuffer.size()));
}

void FUECompressedAnimData::Bind(std::span<const std::byte> BulkData) {
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
    SerializeTrackToSkeletonMapTable(Ar);
    const int32 CurveCount = Ar.ReadCount(8);
    IndexedCurveNames.SetNum(CurveCount);
    for (int32 i = 0; i < CurveCount; i++)
        Ar << IndexedCurveNames[i].CurveName;
    int32 NumBytes = 0;
    bool bUseBulkDataForLoad = false;
    Ar << NumBytes << bUseBulkDataForLoad;
    SerializeByteStream(Ar, NumBytes, bUseBulkDataForLoad);
    Ar << BoneCodecDDCHandle;
    SerializeCurveData(Ar, CurveCompressionSettings);
    if (Ar.IsError())
        return;

    BoneCompressionCodec = BoneCompressionSettings ? BoneCompressionSettings->GetCodec(BoneCodecDDCHandle) : nullptr;
    if (BoneCompressionCodec != nullptr)
        CompressedDataStructure = BoneCompressionCodec->AllocateAnimData();
    if (CompressedDataStructure != nullptr) {
        CompressedDataStructure->SerializeCompressedData(Ar);
        CompressedDataStructure->Bind(std::span<const std::byte>(CompressedByteStream.data(), CompressedByteStream.size()));
    } else if (!CompressedByteStream.empty()) {
        ReportLoadProblem("anim.bone_codec_missing", BoneCodecDDCHandle);
        Ar.SkipBytes(Ar.GetRemaining());
    }
    RebuildCurveIndexTable();
}

void FCompressedAnimSequence::SerializeCompressedDataUE4(FArchive& Ar, UAnimCurveCompressionSettings* CurveCompressionSettings) {
    Ar << CompressedRawDataSize;
    SerializeTrackToSkeletonMapTable(Ar);
    SerializeCurveNames(Ar);
    std::unique_ptr<FUECompressedAnimData> AnimData = std::make_unique<FUECompressedAnimData>();
    AnimData->SerializeCompressedDataUE4(Ar);
    int32 NumBytes = 0;
    bool bUseBulkDataForLoad = false;
    Ar << NumBytes;
    if (Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::FortMappedCookedAnimation, EGame::GAME_UE4_23))
        Ar << bUseBulkDataForLoad;
    SerializeByteStream(Ar, NumBytes, bUseBulkDataForLoad);
    SerializeCurveData(Ar, CurveCompressionSettings);
    if (Ar.IsError())
        return;
    AnimData->Bind(std::span<const std::byte>(CompressedByteStream.data(), CompressedByteStream.size()));
    CompressedDataStructure = std::move(AnimData);
    BoneCompressionCodec = Cast<UAnimCompress>(UAnimCompress::StaticClass()->GetDefaultObject());
    RebuildCurveIndexTable();
}

void FCompressedAnimSequence::SerializeTrackToSkeletonMapTable(FArchive& Ar) {
    const int32 TrackCount = Ar.ReadCount(4);
    CompressedTrackToSkeletonMapTable.SetNum(TrackCount);
    for (int32 i = 0; i < TrackCount; i++)
        Ar << CompressedTrackToSkeletonMapTable[i].BoneTreeIndex;
}

void FCompressedAnimSequence::SerializeCurveNames(FArchive& Ar) {
    const int32 CurveCount = Ar.ReadCount(8);
    IndexedCurveNames.SetNum(CurveCount);
    for (int32 i = 0; i < CurveCount && !Ar.IsError(); i++) {
        FSmartName CurveName;
        CurveName.Serialize(Ar);
        IndexedCurveNames[i].CurveName = CurveName.DisplayName;
    }
}

void FCompressedAnimSequence::SerializeCurveData(FArchive& Ar, UAnimCurveCompressionSettings* CurveCompressionSettings) {
    Ar << CurveCodecPath;
    const int32 NumCurveBytes = Ar.ReadCount(1);
    CompressedCurveByteStream.SetNum(NumCurveBytes);
    if (NumCurveBytes > 0)
        Ar.Serialize(CompressedCurveByteStream.GetData(), NumCurveBytes);
    if (!Ar.IsError())
        CurveCompressionCodec = CurveCompressionSettings ? CurveCompressionSettings->GetCodec(CurveCodecPath) : nullptr;
}

void FCompressedAnimSequence::SerializeByteStream(FArchive& Ar, int32 NumBytes, bool bUseBulkDataForLoad) {
    if (Ar.IsError())
        return;
    //! World of Jade Dynasty swaps the outer bytes of the byte stream size.
    if (Ar.GetGame() == EGame::GAME_WorldofJadeDynasty) {
        const uint32 Swapped = static_cast<uint32>(NumBytes);
        NumBytes = static_cast<int32>((Swapped << 24) | (Swapped & 0xFFFF00u) | (Swapped >> 24));
    }
    if (Ar.GetGame() == EGame::GAME_RocoKingdomWorld) {
        Ar.SkipBytes(16);
        NumBytes -= 16;
    }
    if (NumBytes < 0) {
        Ar.SetError(ErrorCode::InvalidData, "FCompressedAnimSequence.NumBytes");
        return;
    }
    if (!bUseBulkDataForLoad) {
        std::span<const std::byte> Bytes = Ar.ReadView(NumBytes);
        CompressedByteStream.assign(Bytes.begin(), Bytes.end());
        return;
    }
    FByteBulkData OptionalBulk;
    OptionalBulk.Serialize(Ar);
    CompressedByteStream.resize(OptionalBulk.GetPayloadSize());
    Result<void> Read = OptionalBulk.ReadInto(CompressedByteStream);
    if (!Read)
        Ar.SetError(Read.error().code, "FCompressedAnimSequence.BulkData");
}

void FCompressedAnimSequence::RebuildCurveIndexTable() {
    TArray<int32> IndexArray;
    IndexArray.SetNum(IndexedCurveNames.Num());
    for (int32 NameIndex = 0; NameIndex < IndexArray.Num(); ++NameIndex)
        IndexArray[NameIndex] = NameIndex;
    std::sort(IndexArray.begin(), IndexArray.end(), [this](int32 LHS, int32 RHS) {
        const FName& A = IndexedCurveNames[LHS].CurveName;
        const FName& B = IndexedCurveNames[RHS].CurveName;
        return A.getHash() != B.getHash() ? A.getHash() < B.getHash() : A.getNumber() < B.getNumber();
    });
    for (int32 NameIndex = 0; NameIndex < IndexedCurveNames.Num(); ++NameIndex)
        IndexedCurveNames[NameIndex].CurveIndex = IndexArray[NameIndex];
}

}
