#include "Engine/Source/Runtime/Engine/Engine/Level.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Engine/World.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FStreamableTextureInstance& Instance) {
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::STREAMABLE_TEXTURE_AABB) {
        Ar << Instance.Bounds;
    } else {
        FSphere BoundingSphere;
        Ar << BoundingSphere;
        Instance.Bounds = FBoxSphereBounds(BoundingSphere.Center, FVector(BoundingSphere.W), BoundingSphere.W);
    }
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::STREAMABLE_TEXTURE_MIN_MAX_DISTANCE)
        Ar << Instance.MinDistance << Instance.MaxDistance;
    return Ar << Instance.TexelFactor;
}

FArchive& operator<<(FArchive& Ar, FDynamicTextureInstance& Instance) {
    Ar << static_cast<FStreamableTextureInstance&>(Instance);
    return Ar << Instance.Texture << Instance.bAttached << Instance.OriginalRadius;
}

namespace {

template <class TValue>
void SerializeObjectMap(FArchive& Ar) {
    const int32 Count = Ar.ReadCount(8);
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        UObject* Key = nullptr;
        TValue Value{};
        Ar << Key << Value;
    }
}

void SerializeLegacyTextureStreaming(FArchive& Ar) {
    SerializeObjectMap<TArray<FStreamableTextureInstance>>(Ar);
    SerializeObjectMap<TArray<FDynamicTextureInstance>>(Ar);
    bool bIsCooked = false;
    Ar << bIsCooked;
    uint32 LegacySize = 0;
    Ar << LegacySize;
    Ar.SkipBytes(LegacySize);
    SerializeObjectMap<bool>(Ar);
}

void SerializeLegacyLightVolume(FArchive& Ar) {
    bool bInitialized = false;
    Ar << bInitialized;
    if (!bInitialized || Ar.IsError())
        return;
    FBox Bounds;
    float SampleSpacing = 0;
    Ar << Bounds << SampleSpacing;
    int32 NumSHSamples = 4;
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::IndirectLightingCache3BandSupport, EGame::GAME_UE4_12))
        Ar << NumSHSamples;
    int32 SampleSize = 3 * 4 + 4 + NumSHSamples * 3 * 4;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::SKY_BENT_NORMAL)
        SampleSize += 4;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::VOLUME_SAMPLE_LOW_QUALITY_SUPPORT)
        SampleSize += 4;
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(SampleSize)) * SampleSize);
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::VOLUME_SAMPLE_LOW_QUALITY_SUPPORT)
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(SampleSize)) * SampleSize);
}

void SerializePrecomputedVisibility(FArchive& Ar) {
    FVector2D CellBucketOriginXY;
    float CellSizeXY = 0;
    float CellSizeZ = 0;
    int32 CellBucketSizeXY = 0;
    int32 NumCellBuckets = 0;
    Ar << CellBucketOriginXY << CellSizeXY << CellSizeZ << CellBucketSizeXY << NumCellBuckets;
    const int32 BucketCount = Ar.ReadCount(12);
    for (int32 i = 0; i < BucketCount && !Ar.IsError(); i++) {
        int32 CellDataSize = 0;
        Ar << CellDataSize;
        const int32 CellCount = Ar.ReadCount(16);
        for (int32 c = 0; c < CellCount && !Ar.IsError(); c++) {
            FVector Min;
            uint16 ChunkIndex = 0;
            uint16 DataOffset = 0;
            Ar << Min << ChunkIndex << DataOffset;
        }
        const int32 ChunkCount = Ar.ReadCount(12);
        for (int32 c = 0; c < ChunkCount && !Ar.IsError(); c++) {
            bool bCompressed = false;
            int32 UncompressedSize = 0;
            Ar << bCompressed << UncompressedSize;
            Ar.SkipBytes(Ar.ReadCount(1));
        }
    }
}

void SerializePrecomputedVolumeDistanceField(FArchive& Ar) {
    float VolumeMaxDistance = 0;
    FBox VolumeBox;
    int32 VolumeSizeX = 0;
    int32 VolumeSizeY = 0;
    int32 VolumeSizeZ = 0;
    Ar << VolumeMaxDistance << VolumeBox << VolumeSizeX << VolumeSizeY << VolumeSizeZ;
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount(4)) * 4);
}

}

void ULevel::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd())
        return;
    if (!Ar.CustomVerAtLeast(FReleaseObjectVersion::GUID, FReleaseObjectVersion::LevelTransArrayConvertedToTArray, EGame::GAME_UE4_13)) {
        UObject* ActorsOwner = nullptr;
        Ar << ActorsOwner;
    }
    const int32 ActorCount = Ar.ReadCount(4);
    Actors.Empty(ActorCount);
    for (int32 i = 0; i < ActorCount && !Ar.IsError(); i++) {
        UObject* Actor = nullptr;
        Ar << Actor;
        Actors.Add(Cast<AActor>(Actor));
    }
    Ar << URL;
    Ar << Model << ModelComponents << LevelScriptActor;
    if (!Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::RemovedTextureStreamingLevelData, EGame::GAME_UE4_13))
        SerializeLegacyTextureStreaming(Ar);
    Ar << NavListStart << NavListEnd;
    if (Ar.GetGame() == EGame::GAME_MetroAwakening && bIsLightingScenario)
        return;
    if (!Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::MapBuildDataSeparatePackage, EGame::GAME_UE4_14))
        SerializeLegacyLightVolume(Ar);
    SerializePrecomputedVisibility(Ar);
    SerializePrecomputedVolumeDistanceField(Ar);
}

void ULevel::PostLoad() {
    Super::PostLoad();
    if (OwningWorld == nullptr)
        OwningWorld = Cast<UWorld>(GetOuter());
}

}
