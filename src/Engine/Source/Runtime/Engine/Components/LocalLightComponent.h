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
    float AttenuationRadius = 1000;

    ELightUnits GetLightUnits() const override { return IntensityUnits; }
};

}
