#pragma once

#include "Engine/Source/Runtime/Engine/Components/LightComponentBase.h"
#include "Engine/Source/Runtime/Engine/Engine/Scene.h"

#include "LightComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;
class UTextureLightProfile;

UCLASS(Abstract)
class ULightComponent : public ULightComponentBase {
    GENERATED_BODY()
public:
    explicit ULightComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float SourceRadius_DEPRECATED = 0;
    UPROPERTY()
    float Temperature = 6500.0f;
    UPROPERTY()
    float MaxDrawDistance = 0.0f;
    UPROPERTY()
    float MaxDistanceFadeRange = 0.0f;
    UPROPERTY()
    uint8 bUseTemperature : 1 = false;
    UPROPERTY()
    int32 ShadowMapChannel_DEPRECATED = 0;
    UPROPERTY()
    int32 PreviewShadowMapChannel = INDEX_NONE;
    UPROPERTY()
    float MinRoughness_DEPRECATED = 0;
    UPROPERTY()
    float SpecularScale = 1.0f;
    UPROPERTY()
    float DiffuseScale = 1.0f;
    UPROPERTY()
    float ShadowResolutionScale = 1.0f;
    UPROPERTY()
    float ShadowBias = 0.5f;
    UPROPERTY()
    float ShadowSlopeBias = 0.5f;
    UPROPERTY()
    float ShadowSharpen = 0.0f;
    UPROPERTY()
    float ContactShadowLength = 0.0f;
    UPROPERTY()
    uint8 ContactShadowLengthInWS : 1 = false;
    UPROPERTY()
    float ContactShadowCastingIntensity = 1.0f;
    UPROPERTY()
    float ContactShadowNonCastingIntensity = 0.0f;
    UPROPERTY()
    uint8 InverseSquaredFalloff_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bEnabled_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 CastTranslucentShadows : 1 = false;
    UPROPERTY()
    uint8 bAffectDynamicIndirectLighting : 1 = false;
    UPROPERTY()
    uint8 bCastShadowsFromCinematicObjectsOnly : 1 = false;
    UPROPERTY()
    uint8 bForceCachedShadowsForMovablePrimitives : 1 = false;
    UPROPERTY()
    uint8 bAllowMegaLights : 1 = true;
    UPROPERTY()
    TEnumAsByte<EMegaLightsShadowMethod::Type> MegaLightsShadowMethod = EMegaLightsShadowMethod::Default;
    UPROPERTY()
    uint8 bOverrideRayEndBias : 1 = false;
    UPROPERTY()
    float RayEndBias = 0.0f;
    UPROPERTY()
    FLightingChannels LightingChannels;
    UPROPERTY()
    FViewLightingChannels ViewLightingChannels;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> LightFunctionMaterial = nullptr;
    UPROPERTY()
    FVector LightFunctionScale = FVector(1024.0f, 1024.0f, 1024.0f);
    UPROPERTY()
    TObjectPtr<UTextureLightProfile> IESTexture = nullptr;
    UPROPERTY()
    uint8 bUseIESBrightness : 1 = false;
    UPROPERTY()
    float IESBrightnessScale = 1.0f;
    UPROPERTY()
    float LightFunctionFadeDistance = 100000.0f;
    UPROPERTY()
    float DisabledBrightness = 0.5f;
    UPROPERTY()
    uint8 bEnableLightShaftBloom : 1 = false;
    UPROPERTY()
    float BloomScale = 0.2f;
    UPROPERTY()
    float BloomThreshold = 0;
    UPROPERTY()
    float BloomMaxBrightness = 100.0f;
    UPROPERTY()
    FColor BloomTint = FColor::White;
    UPROPERTY()
    bool bUseRayTracedDistanceFieldShadows = false;
    UPROPERTY()
    float RayStartOffsetDepthScale = 0.003f;

    void PostLoad() override;
    virtual ELightUnits GetLightUnits() const;
    virtual float ComputeLightBrightness() const;
};

}
