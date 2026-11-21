#pragma once

#include "Engine/Source/Runtime/Core/Math/Color.h"
#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "ExponentialHeightFogComponent.generated.h"

namespace ue5parse::runtime {

class UTextureCube;

USTRUCT()
struct FExponentialHeightFogData {
    GENERATED_BODY()

    UPROPERTY()
    float FogDensity = 0.02f;
    UPROPERTY()
    float FogHeightFalloff = 0.2f;
    UPROPERTY()
    float FogHeightOffset = 0.0f;
};

UCLASS()
class UExponentialHeightFogComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UExponentialHeightFogComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float FogDensity = 0.02f;
    UPROPERTY()
    float FogHeightFalloff = 0.2f;
    UPROPERTY()
    FExponentialHeightFogData SecondFogData;
    UPROPERTY()
    FLinearColor FogInscatteringColor_DEPRECATED{0.447f, 0.638f, 1.0f};
    UPROPERTY()
    FLinearColor FogInscatteringLuminance{0.0f, 0.0f, 0.0f};
    UPROPERTY()
    FLinearColor SkyAtmosphereAmbientContributionColorScale{1.0f, 1.0f, 1.0f};
    UPROPERTY()
    TObjectPtr<UTextureCube> InscatteringColorCubemap = nullptr;
    UPROPERTY()
    float InscatteringColorCubemapAngle = 0.0f;
    UPROPERTY()
    FLinearColor InscatteringTextureTint{1.0f, 1.0f, 1.0f};
    UPROPERTY()
    float FullyDirectionalInscatteringColorDistance = 100000.0f;
    UPROPERTY()
    float NonDirectionalInscatteringColorDistance = 1000.0f;
    UPROPERTY()
    float DirectionalInscatteringExponent = 4.0f;
    UPROPERTY()
    float DirectionalInscatteringStartDistance = 10000.0f;
    UPROPERTY()
    FLinearColor DirectionalInscatteringColor_DEPRECATED{0.25f, 0.25f, 0.125f};
    UPROPERTY()
    FLinearColor DirectionalInscatteringLuminance{0.0f, 0.0f, 0.0f};
    UPROPERTY()
    float SkyLightCaptureAffectsHeightFogStrength = 0.0f;
    UPROPERTY()
    float SkyLightCaptureAffectsHeightFogRoughness = 0.15f;
    UPROPERTY()
    float FogMaxOpacity = 1.0f;
    UPROPERTY()
    float StartDistance = 0.0f;
    UPROPERTY()
    float EndDistance = 0.0f;
    UPROPERTY()
    float FogCutoffDistance = 0.0f;
    UPROPERTY()
    uint8 bHoldout : 1 = false;
    UPROPERTY()
    uint8 bRenderInMainPass : 1 = true;
    UPROPERTY()
    uint8 bVisibleInReflectionCaptures : 1 = true;
    UPROPERTY()
    uint8 bVisibleInRealTimeSkyCaptures : 1 = true;
    UPROPERTY()
    bool bEnableVolumetricFog = false;
    UPROPERTY()
    float VolumetricFogScatteringDistribution = 0.2f;
    UPROPERTY()
    FColor VolumetricFogAlbedo{255, 255, 255, 255};
    UPROPERTY()
    FLinearColor VolumetricFogEmissive{0.0f, 0.0f, 0.0f};
    UPROPERTY()
    float VolumetricFogExtinctionScale = 1.0f;
    UPROPERTY()
    float VolumetricFogDistance = 6000.0f;
    UPROPERTY()
    float VolumetricFogStartDistance = 0.0f;
    UPROPERTY()
    float VolumetricFogNearFadeInDistance = 0.0f;
    UPROPERTY()
    float VolumetricFogStaticLightingScatteringIntensity = 1.0f;
    UPROPERTY()
    bool bOverrideLightColorsWithFogInscatteringColors = false;
    UPROPERTY()
    bool bEnableFSSS = false;
    UPROPERTY()
    float FSSSSceneColorScatteringAmountScale = 1.0f;
    UPROPERTY()
    float FSSSSceneColorScatteringAmountPower = 1.0f;
    UPROPERTY()
    float FSSSSpreadScale = 0.1f;
    UPROPERTY()
    float FSSSBlurControl = 0.5f;
};

}
