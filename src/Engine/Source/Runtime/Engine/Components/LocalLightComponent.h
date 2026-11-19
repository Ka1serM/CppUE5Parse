#pragma once

#include "Engine/Source/Runtime/Engine/Components/LightComponent.h"

#include "LocalLightComponent.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class ULocalLightComponent : public ULightComponent {
    GENERATED_BODY()
public:
    explicit ULocalLightComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    ELightUnits IntensityUnits = ELightUnits::Unitless;
    UPROPERTY()
    float InverseExposureBlend = 0;
    UPROPERTY()
    float Radius_DEPRECATED = 1024.0f;
    UPROPERTY()
    float AttenuationRadius = 1000;
    UPROPERTY()
    FLightmassPointLightSettings LightmassSettings;

    ELightUnits GetLightUnits() const override { return IntensityUnits; }
};

}
