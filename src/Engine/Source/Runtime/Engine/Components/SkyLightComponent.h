#pragma once

#include "Engine/Source/Runtime/Engine/Components/LightComponentBase.h"

#include "SkyLightComponent.generated.h"

namespace ue5parse::runtime {

class UTextureCube;

UENUM()
enum ESkyLightSourceType : int32 { SLS_CapturedScene, SLS_SpecifiedCubemap, SLS_MAX };

UCLASS()
class USkyLightComponent : public ULightComponentBase {
    GENERATED_BODY()
public:
    explicit USkyLightComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    bool bRealTimeCapture = false;
    UPROPERTY()
    TEnumAsByte<ESkyLightSourceType> SourceType = SLS_CapturedScene;
    UPROPERTY()
    TObjectPtr<UTextureCube> Cubemap = nullptr;
    UPROPERTY()
    float SourceCubemapAngle = 0;
    UPROPERTY()
    int32 CubemapResolution = 128;
    UPROPERTY()
    float SkyDistanceThreshold = 150000;
    UPROPERTY()
    bool bCaptureEmissiveOnly = false;
    UPROPERTY()
    bool bLowerHemisphereIsBlack = true;
    UPROPERTY()
    FLinearColor LowerHemisphereColor = FLinearColor::Black;
    UPROPERTY()
    float OcclusionMaxDistance = 1000;
    UPROPERTY()
    float Contrast = 0;
    UPROPERTY()
    float OcclusionExponent = 1;
    UPROPERTY()
    float MinOcclusion = 0;
    UPROPERTY()
    FColor OcclusionTint = FColor::Black;
    UPROPERTY()
    uint8 bCloudAmbientOcclusion : 1 = false;
    UPROPERTY()
    float CloudAmbientOcclusionStrength = 1.0f;
    UPROPERTY()
    float CloudAmbientOcclusionExtent = 150.0f;
    UPROPERTY()
    float CloudAmbientOcclusionMapResolutionScale = 1.0f;
    UPROPERTY()
    float CloudAmbientOcclusionApertureScale = 0.05f;
    UPROPERTY()
    TEnumAsByte<EOcclusionCombineMode> OcclusionCombineMode = OCM_Minimum;
};

}
