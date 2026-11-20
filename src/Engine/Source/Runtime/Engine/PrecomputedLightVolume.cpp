#include "Engine/Source/Runtime/Engine/PrecomputedLightVolume.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

namespace {

template <int32 SHOrder>
void LoadConvertedSamples(FArchive& Ar, TArray<FVolumeLightingSample>& Samples) {
    TArray<TVolumeLightingSample<SHOrder>> LoadedSamples;
    Ar << LoadedSamples;
    Samples.Empty(LoadedSamples.Num());
    for (const TVolumeLightingSample<SHOrder>& LoadedSample : LoadedSamples)
        Samples.Add(FVolumeLightingSample(LoadedSample));
}

void LoadVolumeLightSamples(FArchive& Ar, int32 ArchiveNumSHSamples, TArray<FVolumeLightingSample>& Samples) {
    if (ArchiveNumSHSamples == 9) {
        Ar << Samples;
        return;
    }
    if (ArchiveNumSHSamples != 4) {
        Ar.SetError(ErrorCode::InvalidData, "FPrecomputedLightVolumeData.NumSHSamples");
        return;
    }
    LoadConvertedSamples<2>(Ar, Samples);
}

}

FArchive& operator<<(FArchive& Ar, TVolumeLightingSample<2>& Sample) {
    Ar << Sample.Position << Sample.Radius << Sample.Lighting;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::SKY_BENT_NORMAL)
        Ar << Sample.PackedSkyBentNormal;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::VOLUME_SAMPLE_LOW_QUALITY_SUPPORT)
        Ar << Sample.DirectionalLightShadowing;
    return Ar;
}

FArchive& operator<<(FArchive& Ar, TVolumeLightingSample<3>& Sample) {
    return Ar << Sample.Position << Sample.Radius << Sample.Lighting << Sample.PackedSkyBentNormal << Sample.DirectionalLightShadowing;
}

FArchive& operator<<(FArchive& Ar, FPrecomputedLightVolumeData& Volume) {
    Ar << Volume.bInitialized;
    if (!Volume.bInitialized)
        return Ar;
    float SampleSpacing = 0.0f;
    Ar << Volume.Bounds << SampleSpacing;
    int32 NumSHSamples = 4;
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::IndirectLightingCache3BandSupport, EGame::GAME_UE4_12))
        Ar << NumSHSamples;
    LoadVolumeLightSamples(Ar, NumSHSamples, Volume.HighQualitySamples);
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::VOLUME_SAMPLE_LOW_QUALITY_SUPPORT)
        LoadVolumeLightSamples(Ar, NumSHSamples, Volume.LowQualitySamples);
    return Ar;
}

FArchive& operator<<(FArchive& Ar, std::unique_ptr<FPrecomputedLightVolumeData>& Volume) {
    bool bValid = false;
    Ar << bValid;
    Volume.reset();
    if (!bValid)
        return Ar;
    Volume = std::make_unique<FPrecomputedLightVolumeData>();
    return Ar << *Volume;
}

}
