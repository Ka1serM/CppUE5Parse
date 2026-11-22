#include "Engine/Source/Runtime/Engine/ShadowMap.h"

#include "Engine/Source/Runtime/Engine/Engine/ShadowMapTexture2D.h"

namespace ue5parse::runtime {

UShadowMapTexture2D::UShadowMapTexture2D(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    LODGroup = TEXTUREGROUP_Shadowmap;
}

void FShadowMap2D::Serialize(FArchive& Ar) {
    Ar << LightGuids << Texture << CoordinateScale << CoordinateBias;
    for (bool& bValid : bChannelValid)
        Ar << bValid;
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::STATIC_SHADOWMAP_PENUMBRA_SIZE) {
        Ar << InvUniformPenumbraSize;
    } else {
        constexpr float LegacyValue = 1.0f / .05f;
        InvUniformPenumbraSize = FVector4f(LegacyValue, LegacyValue, LegacyValue, LegacyValue);
    }
    if (Ar.GetGame() == EGame::GAME_Snowbreak)
        Ar.SkipBytes(20);
}

FArchive& operator<<(FArchive& Ar, FShadowMapRef& ShadowMap) {
    uint32 ShadowMapType = FShadowMap::SMT_None;
    Ar << ShadowMapType;
    ShadowMap.reset();
    if (ShadowMapType == FShadowMap::SMT_None)
        return Ar;
    if (ShadowMapType != FShadowMap::SMT_2D) {
        Ar.SetError(ErrorCode::UnsupportedVersion, "FShadowMap.ShadowMapType");
        return Ar;
    }
    ShadowMap = std::make_shared<FShadowMap2D>();
    ShadowMap->Serialize(Ar);
    return Ar;
}

}
