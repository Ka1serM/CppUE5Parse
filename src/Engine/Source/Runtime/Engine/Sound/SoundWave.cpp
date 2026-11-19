#include "Engine/Source/Runtime/Engine/Sound/SoundWave.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

using V4 = EUnrealEngineObjectUE4Version;

constexpr uint32 CookedFlag = 1 << 0;
constexpr uint32 HasOwnerLoadingBehaviorFlag = 1 << 1;
constexpr uint32 LoadingBehaviorShift = 2;
constexpr uint32 LoadingBehaviorMask = 0b111;

constexpr uint32 ChunkIsCooked = 1 << 0;
constexpr uint32 ChunkHasSeekOffset = 1 << 1;

}

void FStreamedAudioChunk::Serialize(FArchive& Ar, int32 ChunkIndex) {
    const EGame Game = Ar.GetGame();
    uint32 Flags = 0;
    if (Game >= EGame::GAME_UE5_3) {
        Ar << Flags;
    } else {
        bool bCooked = false;
        Ar << bCooked;
        Flags = bCooked ? ChunkIsCooked : 0;
    }
    BulkData.Serialize(Ar);
    if (Game >= EGame::GAME_UE4_4)
        Ar << DataSize;
    if (Game >= EGame::GAME_UE4_19)
        Ar << AudioDataSize;
    if (Flags & ChunkHasSeekOffset)
        Ar << SeekOffsetInAudioFrames;
    if (!(Flags & ChunkIsCooked)) {
        FString DerivedDataKey;
        Ar << DerivedDataKey;
    }
}

void FStreamedAudioPlatformData::Serialize(FArchive& Ar) {
    const int32 NumChunks = Ar.ReadCount(1);
    Ar << AudioFormat;
    Chunks.Empty(NumChunks);
    for (int32 ChunkIndex = 0; ChunkIndex < NumChunks && !Ar.IsError(); ChunkIndex++)
        Chunks.Emplace_GetRef().Serialize(Ar, ChunkIndex);
}

void USoundWave::SerializeCuePoints(FArchive& Ar, bool bIsLoadingFromCookedArchive) {
    if (!bIsLoadingFromCookedArchive)
        return;
    const int32 Count = Ar.ReadCount(1);
    PlatformCuePoints.Empty(Count);
    for (int32 Index = 0; Index < Count && !Ar.IsError(); Index++)
        FSoundWaveCuePoint::StaticStruct()->SerializeItem(Ar, &PlatformCuePoints.Emplace_GetRef());
}

void USoundWave::SerializeCookedPlatformData(FArchive& Ar) {
    RunningPlatformData = std::make_unique<FStreamedAudioPlatformData>();
    RunningPlatformData->Serialize(Ar);
}

bool USoundWave::IsStreaming(const FArchive& Ar) const {
    //! The default cook overrides neither cache streams nor set an auto streaming threshold.
    if (Ar.GetGame() < EGame::GAME_UE5_0)
        return bStreaming;
    return LoadingBehavior != ESoundWaveLoadingBehavior::ForceInline && !bProcedural;
}

void USoundWave::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    const EGame Game = Ar.GetGame();
    uint32 Flags = 0;
    if (Game >= EGame::GAME_UE5_4) {
        Ar << Flags;
    } else {
        bool bCookedValue = false;
        Ar << bCookedValue;
        Flags = bCookedValue ? CookedFlag : 0;
    }
    const bool bCooked = Flags & CookedFlag;
    if (Ar.UEVer() >= V4::SOUND_COMPRESSION_TYPE_ADDED &&
        !Ar.CustomVerAtLeast(FFrameworkObjectVersion::GUID, FFrameworkObjectVersion::RemoveSoundWaveCompressionName, EGame::GAME_UE4_13)) {
        FName CompressionName;
        Ar << CompressionName;
    }
    const bool bShouldStreamSound = IsStreaming(Ar);
    if (Game >= EGame::GAME_UE5_4)
        SerializeCuePoints(Ar, bCooked);
    //! In 4.3 the cooker saved the compressed formats of streaming sounds too.
    if (bCooked && (!bShouldStreamSound || Game < EGame::GAME_UE4_4))
        CompressedFormatData.Serialize(Ar);
    if (!bCooked) {
        //! Since SoundWaveVirtualizationUpdate the raw data is an FEditorBulkData, which is not ported.
        if (Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::SoundWaveVirtualizationUpdate, EGame::GAME_UE5_0)) {
            Ar.SetError(ErrorCode::UnsupportedVersion, "USoundWave.RawData");
            return;
        }
        FByteBulkData RawData;
        RawData.Serialize(Ar);
    }
    Ar << CompressedDataGuid;
    if (bCooked && (Flags & HasOwnerLoadingBehaviorFlag) && LoadingBehavior != ESoundWaveLoadingBehavior::ForceInline)
        LoadingBehavior = static_cast<ESoundWaveLoadingBehavior>((Flags >> LoadingBehaviorShift) & LoadingBehaviorMask);
    if (bShouldStreamSound && bCooked)
        SerializeCookedPlatformData(Ar);
}

}
