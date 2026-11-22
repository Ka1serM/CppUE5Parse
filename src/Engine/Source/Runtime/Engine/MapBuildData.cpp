#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Engine/MapBuildDataRegistry.h"
#include "Engine/Source/Runtime/Engine/EngineUtils.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FMeshMapBuildData& MeshMapBuildData) {
    Ar << MeshMapBuildData.LightMap << MeshMapBuildData.ShadowMap << MeshMapBuildData.IrrelevantLights;
    BulkSerialize(Ar, MeshMapBuildData.PerInstanceLightmapData);
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FStaticShadowDepthMapData& ShadowMap) {
    return Ar << ShadowMap.WorldToLight << ShadowMap.ShadowMapSizeX << ShadowMap.ShadowMapSizeY << ShadowMap.DepthSamples;
}

FArchive& operator<<(FArchive& Ar, FLightComponentMapBuildData& LightBuildData) {
    return Ar << LightBuildData.ShadowMapChannel << LightBuildData.DepthMap;
}

FArchive& operator<<(FArchive& Ar, FReflectionCaptureMapBuildData& ReflectionCaptureMapBuildData) {
    Ar << ReflectionCaptureMapBuildData.CubemapSize << ReflectionCaptureMapBuildData.AverageBrightness;
    const bool bStoresBrightness =
        Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::StoreReflectionCaptureBrightnessForCooking, EGame::GAME_UE4_19);
    const bool bExcludesBrightness =
        Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID, FUE5ReleaseStreamObjectVersion::ExcludeBrightnessFromEncodedHDRCubemap, EGame::GAME_UE5_0);
    if (bStoresBrightness && !bExcludesBrightness)
        Ar << ReflectionCaptureMapBuildData.Brightness;
    Ar << ReflectionCaptureMapBuildData.FullHDRCapturedData;
    const bool bStoresTextureCube =
        Ar.CustomVerAtLeast(FMobileObjectVersion::GUID, FMobileObjectVersion::StoreReflectionCaptureCompressedMobile, EGame::GAME_UE4_26);
    const bool bStoresRG11B10Data = Ar.CustomVerAtLeast(FUE5ReleaseStreamObjectVersion::GUID,
                                                        FUE5ReleaseStreamObjectVersion::StoreReflectionCaptureEncodedHDRDataInRG11B10Format, EGame::GAME_UE5_1);
    if (bStoresTextureCube && !bStoresRG11B10Data)
        Ar << ReflectionCaptureMapBuildData.EncodedCaptureData;
    else
        Ar << ReflectionCaptureMapBuildData.EncodedHDRCapturedData;
    //! Valorant appends a byte array to the reflection capture data.
    if (Ar.GetGame() == EGame::GAME_Valorant)
        Ar.SkipBytes(static_cast<int64>(Ar.ReadCount()));
    if (Ar.GetGame() != EGame::GAME_BlackMythWukong)
        return Ar;
    //! Black Myth: Wukong appends a byte array and four bytes to the reflection capture data.
    Ar.SkipBytes(static_cast<int64>(Ar.ReadCount()) + 4);
    return Ar;
}

void UMapBuildDataRegistry::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    FStripDataFlags StripFlags(Ar);
    if (StripFlags.IsAudioVisualDataStripped())
        return;
    Ar << MeshBuildData << LevelPrecomputedLightVolumeBuildData;
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::VolumetricLightmaps, EGame::GAME_UE4_18))
        Ar << LevelPrecomputedVolumetricLightmapBuildData;
    Ar << LightBuildData;
    if (Ar.CustomVerAtLeast(FReflectionCaptureObjectVersion::GUID, FReflectionCaptureObjectVersion::MoveReflectionCaptureDataToMapBuildData,
                            EGame::GAME_UE4_19))
        Ar << ReflectionCaptureBuildData;
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::SkyAtmosphereStaticLightingVersioning, EGame::GAME_UE4_24))
        Ar << SkyAtmosphereBuildData;
    if (!Ar.CustomVerAtLeast(FFortniteMainBranchObjectVersion::GUID, FFortniteMainBranchObjectVersion::VolumetricLightMapGridDescSupport, EGame::GAME_UE5_5))
        return;
    bool bHasGrid = false;
    Ar << bHasGrid;
    if (!bHasGrid)
        return;
    VolumetricLightMapGridDesc = std::make_unique<FVolumetricLightMapGridDesc>();
    FVolumetricLightMapGridDesc::StaticStruct()->SerializeItem(Ar, VolumetricLightMapGridDesc.get());
    VolumetricLightMapGridDesc->SerializeBulkData(Ar);
}

}
