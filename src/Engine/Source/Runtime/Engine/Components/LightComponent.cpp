#include "Engine/Source/Runtime/Engine/Components/LightComponent.h"

#include <algorithm>
#include <cmath>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Core/UObject/DevObjectVersion.h"
#include "Engine/Source/Runtime/Engine/Engine/TextureLightProfile.h"
#include "Engine/Source/Runtime/RenderCore/RenderUtils.h"

namespace ue5parse::runtime {

ULightComponent::ULightComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bAffectTranslucentLighting = true;
}

namespace {

//! Before MapBuildDataSeparatePackage the light stores its FStaticShadowDepthMapData inline.
void SkipStaticShadowDepthMapData(FArchive& Ar) {
    FMatrix44f WorldToLight;
    for (float(&Row)[4] : WorldToLight.M)
        for (float& Value : Row)
            Ar << Value;
    int32 ShadowMapSizeX = 0;
    int32 ShadowMapSizeY = 0;
    TArray<uint16> DepthSamples;
    Ar << ShadowMapSizeX << ShadowMapSizeY << DepthSamples;
}

}

void ULightComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::STATIC_SHADOW_DEPTH_MAPS
        && !Ar.CustomVerAtLeast(FRenderingObjectVersion::GUID, FRenderingObjectVersion::MapBuildDataSeparatePackage, EGame::GAME_UE4_14))
        SkipStaticShadowDepthMapData(Ar);
    if (Ar.GetGame() == EGame::GAME_Valorant)
        Ar.SkipBytes(24);
}

void ULightComponent::PostLoad() {
    Super::PostLoad();
    Intensity = std::max(0.0f, Intensity);
}

ELightUnits ULightComponent::GetLightUnits() const {
    return ELightUnits::Unitless;
}

float ULightComponent::ComputeLightBrightness() const {
    float LightBrightness = Intensity;
    if (IESTexture != nullptr) {
        const bool bEVUnit = GetLightUnits() == ELightUnits::EV;
        if (bEVUnit)
            LightBrightness = EV100ToLuminance(Intensity);
        if (bUseIESBrightness)
            LightBrightness = IESTexture->Brightness * IESBrightnessScale;
        LightBrightness *= IESTexture->TextureMultiplier;
        if (bEVUnit)
            LightBrightness = LuminanceToEV100(LightBrightness);
    }
    return LightBrightness;
}

}
