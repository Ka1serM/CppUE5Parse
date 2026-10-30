#pragma once

#include "ue5parse/runtime/Engine/Components/LightComponent.h"

#include "LocalLightComponent.generated.h"

namespace ue5parse::runtime {

// ULocalLightComponent: a light with a position and a radius of influence (UE's Components/LocalLightComponent.h).
UCLASS(Abstract)
class ULocalLightComponent : public ULightComponent {
    GENERATED_BODY()
public:
    explicit ULocalLightComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    ELightUnits IntensityUnits = ELightUnits::Unitless;
    UPROPERTY()
    float AttenuationRadius = 1000;

    ELightUnits GetLightUnits() const override { return IntensityUnits; }
    // The factor that converts an intensity between two units; CosHalfConeAngle for a spot light's lumens.
    static float GetUnitsConversionFactor(ELightUnits SrcUnits, ELightUnits TargetUnits, float CosHalfConeAngle = -1);
};

}
