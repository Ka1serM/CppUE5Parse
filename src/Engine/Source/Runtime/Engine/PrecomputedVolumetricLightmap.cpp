#include "Engine/Source/Runtime/Engine/PrecomputedVolumetricLightmap.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Engine/Texture.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FVolumetricLightmapDataLayer& Layer) {
    FString PixelFormatString;
    Ar << Layer.Data << PixelFormatString;
    if (Ar.IsError())
        return Ar;
    const int64 PixelFormatValue = UTexture::GetPixelFormatEnum()->GetValueByName(FName(PixelFormatString.ToUtf8()));
    if (PixelFormatValue == INDEX_NONE) {
        Ar.SetError(ErrorCode::InvalidData, "FVolumetricLightmapDataLayer.Format");
        return Ar;
    }
    Layer.Format = static_cast<EPixelFormat>(PixelFormatValue);
    return Ar;
}

FArchive& operator<<(FArchive& Ar, FPrecomputedVolumetricLightmapData& Volume) {
    Ar << Volume.Bounds << Volume.IndirectionTextureDimensions << Volume.IndirectionTexture;
    Ar << Volume.BrickSize << Volume.BrickDataDimensions;
    FVolumetricLightmapBrickData& BrickData = Volume.BrickData;
    Ar << BrickData.AmbientVector;
    for (FVolumetricLightmapDataLayer& Coefficients : BrickData.SHCoefficients)
        Ar << Coefficients;
    Ar << BrickData.SkyBentNormal << BrickData.DirectionalLightShadowing;
    const bool bHasLowQualityLayers =
        Ar.CustomVerAtLeast(FMobileObjectVersion::GUID, FMobileObjectVersion::LQVolumetricLightmapLayers, EGame::GAME_UE4_19)
        && !Ar.CustomVerAtLeast(FUE5MainStreamObjectVersion::GUID, FUE5MainStreamObjectVersion::MobileStationaryLocalLights, EGame::GAME_UE5_1);
    if (bHasLowQualityLayers) {
        FVolumetricLightmapDataLayer LQLightColor;
        FVolumetricLightmapDataLayer LQLightDirection;
        Ar << LQLightColor << LQLightDirection;
    }
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::VolumetricLightmapStreaming, EGame::GAME_UE4_24))
        Ar << Volume.SubLevelBrickPositions << Volume.IndirectionTextureOriginalValues;
    return Ar;
}

FArchive& operator<<(FArchive& Ar, std::unique_ptr<FPrecomputedVolumetricLightmapData>& Volume) {
    bool bValid = false;
    Ar << bValid;
    Volume.reset();
    if (!bValid)
        return Ar;
    Volume = std::make_unique<FPrecomputedVolumetricLightmapData>();
    return Ar << *Volume;
}

}
