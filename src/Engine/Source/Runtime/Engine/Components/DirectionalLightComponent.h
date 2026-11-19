#pragma once

#include "Engine/Source/Runtime/Engine/Components/LightComponent.h"

#include "DirectionalLightComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UDirectionalLightComponent : public ULightComponent {
    GENERATED_BODY()
public:
    explicit UDirectionalLightComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float ShadowCascadeBiasDistribution = 1;
    UPROPERTY()
    uint8 bEnableLightShaftOcclusion : 1 = false;
    UPROPERTY()
    float OcclusionMaskDarkness = 0.05f;
    UPROPERTY()
    float OcclusionDepthRange = 100000.0f;
    UPROPERTY()
    FVector LightShaftOverrideDirection = FVector::ZeroVector;
    UPROPERTY()
    float WholeSceneDynamicShadowRadius_DEPRECATED = 20000.0f;
    UPROPERTY()
    float DynamicShadowDistanceMovableLight = 40000.0f;
    UPROPERTY()
    float DynamicShadowDistanceStationaryLight = 0.0f;
    UPROPERTY()
    int32 DynamicShadowCascades = 4;
    UPROPERTY()
    float CascadeDistributionExponent = 3.0f;
    UPROPERTY()
    float CascadeTransitionFraction = 0.1f;
    UPROPERTY()
    float ShadowDistanceFadeoutFraction = 0.1f;
    UPROPERTY()
    uint8 bUseInsetShadowsForMovableObjects : 1 = true;
    UPROPERTY()
    int32 FarShadowCascadeCount = 0;
    UPROPERTY()
    float FarShadowDistance = 300000.0f;
    UPROPERTY()
    float DistanceFieldShadowDistance = 51200.0f;
    UPROPERTY()
    int32 ForwardShadingPriority = 0;
    UPROPERTY()
    float LightSourceAngle = 0.5357f;
    UPROPERTY()
    float LightSourceSoftAngle = 0.0f;
    UPROPERTY()
    float ShadowSourceAngleFactor = 1.0f;
    UPROPERTY()
    float TraceDistance = 10000.0f;
    UPROPERTY()
    uint8 bUsedAsAtmosphereSunLight_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bAtmosphereSunLight : 1 = true;
    UPROPERTY()
    int32 AtmosphereSunLightIndex = 0;
    UPROPERTY()
    FLinearColor AtmosphereSunDiskColorScale = FLinearColor::White;
    UPROPERTY()
    uint8 bPerPixelAtmosphereTransmittance : 1 = false;
    UPROPERTY()
    uint8 bCastShadowsOnClouds : 1 = false;
    UPROPERTY()
    uint8 bCastShadowsOnAtmosphere : 1 = false;
    UPROPERTY()
    uint8 bCastCloudShadows : 1 = false;
    UPROPERTY()
    float CloudShadowStrength = 1.0f;
    UPROPERTY()
    float CloudShadowOnAtmosphereStrength = 1.0f;
    UPROPERTY()
    float CloudShadowOnSurfaceStrength = 1.0f;
    UPROPERTY()
    float CloudShadowDepthBias = 0.0f;
    UPROPERTY()
    float CloudShadowExtent = 150.0f;
    UPROPERTY()
    float CloudShadowMapResolutionScale = 1.0f;
    UPROPERTY()
    float CloudShadowRaySampleCountScale = 1.0f;
    UPROPERTY()
    FLinearColor CloudScatteredLuminanceScale = FLinearColor::White;
    UPROPERTY()
    FLightmassDirectionalLightSettings LightmassSettings;
    UPROPERTY()
    uint8 bCastModulatedShadows : 1 = false;
    UPROPERTY()
    FColor ModulatedShadowColor = FColor(128, 128, 128);
    UPROPERTY()
    float ShadowAmount = 1.0f;

    ELightUnits GetLightUnits() const override { return ELightUnits::Unitless; }
};

}
