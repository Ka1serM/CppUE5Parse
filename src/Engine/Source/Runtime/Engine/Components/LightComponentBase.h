#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"

#include "LightComponentBase.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class ULightComponentBase : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FGuid OriginalLightGuid;
    UPROPERTY()
    FGuid LightGuid;
    UPROPERTY()
    float Brightness_DEPRECATED = 3.1415926535897932f;
    UPROPERTY()
    float Intensity = 3.1415926535897932f;
    UPROPERTY()
    FColor LightColor = FColor::White;
    UPROPERTY()
    uint8 bAffectsWorld : 1 = true;
    UPROPERTY()
    uint8 CastShadows : 1 = true;
    UPROPERTY()
    uint8 CastStaticShadows : 1 = true;
    UPROPERTY()
    uint8 CastDynamicShadows : 1 = true;
    UPROPERTY()
    uint8 bAffectTranslucentLighting : 1 = false;
    UPROPERTY()
    uint8 bPrecomputedLightingIsValid : 1 = false;
    UPROPERTY()
    uint8 bTransmission : 1 = false;
    UPROPERTY()
    uint8 bCastVolumetricShadow : 1 = false;
    UPROPERTY()
    uint8 bCastDeepShadow : 1 = false;
    UPROPERTY()
    uint8 bCastRaytracedShadow_DEPRECATED : 1 = true;
    UPROPERTY()
    TEnumAsByte<ECastRayTracedShadow::Type> CastRaytracedShadow = ECastRayTracedShadow::UseProjectSetting;
    UPROPERTY()
    uint8 bAffectReflection : 1 = true;
    UPROPERTY()
    uint8 bAffectGlobalIllumination : 1 = true;
    UPROPERTY()
    float DeepShadowLayerDistribution = 0.5f;
    UPROPERTY()
    float IndirectLightingIntensity = 1.0f;
    UPROPERTY()
    float VolumetricScatteringIntensity = 1.0f;
    UPROPERTY()
    int32 SamplesPerPixel = 1;

    void Serialize(FArchive& Ar) override;
};

}
