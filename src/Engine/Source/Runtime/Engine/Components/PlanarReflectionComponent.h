#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneCaptureComponent.h"

#include "PlanarReflectionComponent.generated.h"

namespace ue5parse::runtime {

class UBoxComponent;

UCLASS()
class UPlanarReflectionComponent : public USceneCaptureComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UBoxComponent> PreviewBox = nullptr;
    UPROPERTY()
    float NormalDistortionStrength = 500.0f;
    UPROPERTY()
    float PrefilterRoughness = 0.01f;
    UPROPERTY()
    float PrefilterRoughnessDistance = 10000.0f;
    UPROPERTY()
    int32 ScreenPercentage = 50;
    UPROPERTY()
    float ExtraFOV = 0.0f;
    UPROPERTY()
    float DistanceFromPlaneFadeStart_DEPRECATED = 400.0f;
    UPROPERTY()
    float DistanceFromPlaneFadeEnd_DEPRECATED = 600.0f;
    UPROPERTY()
    float DistanceFromPlaneFadeoutStart = 60.0f;
    UPROPERTY()
    float DistanceFromPlaneFadeoutEnd = 100.0f;
    UPROPERTY()
    float AngleFromPlaneFadeStart = 20.0f;
    UPROPERTY()
    float AngleFromPlaneFadeEnd = 30.0f;
    UPROPERTY()
    bool bRenderSceneTwoSided = false;
    UPROPERTY()
    bool bShowPreviewPlane = true;

    explicit UPlanarReflectionComponent(const FObjectInitializer& ObjectInitializer);
};

}
