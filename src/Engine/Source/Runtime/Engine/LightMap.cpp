#include "Engine/Source/Runtime/Engine/LightMap.h"

#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"

namespace ue5parse::runtime {

void FLightMap2D::Serialize(FArchive& Ar) {
    Ar << LightGuids;
    if (Ar.UEVer() < EUnrealEngineObjectUE4Version::COMBINED_LIGHTMAP_TEXTURES) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FLightMap2D.COMBINED_LIGHTMAP_TEXTURES");
        return;
    }
    Ar << Textures[0] << Textures[1];
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::SKY_LIGHT_COMPONENT) {
        Ar << SkyOcclusionTexture;
        if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::AO_MATERIAL_MASK)
            Ar << AOMaterialMaskTexture;
    }
    for (int32 CoefficientIndex = 0; CoefficientIndex < NUM_STORED_LIGHTMAP_COEF; CoefficientIndex++)
        Ar << ScaleVectors[CoefficientIndex] << AddVectors[CoefficientIndex];
    Ar << CoordinateScale << CoordinateBias;
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::LightmapHasShadowmapData, EGame::GAME_UE4_23)) {
        for (bool& bValid : bShadowChannelValid)
            Ar << bValid;
        Ar << InvUniformPenumbraSize;
    }
    SerializeVirtualTextures(Ar);
}

void FLightMap2D::SerializeVirtualTextures(FArchive& Ar) {
    if (!Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::VirtualTexturedLightmaps, EGame::GAME_UE4_21))
        return;
    Ar << VirtualTextures[0];
    if (Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::VirtualTexturedLightmapsV3, EGame::GAME_UE4_27))
        Ar << VirtualTextures[1];
}

FArchive& operator<<(FArchive& Ar, FLightMapRef& LightMap) {
    uint32 LightMapType = FLightMap::LMT_None;
    Ar << LightMapType;
    LightMap.reset();
    if (LightMapType == FLightMap::LMT_None)
        return Ar;
    if (LightMapType != FLightMap::LMT_2D) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FLightMap.LightMapType");
        return Ar;
    }
    LightMap = std::make_shared<FLightMap2D>();
    LightMap->Serialize(Ar);
    return Ar;
}

}
