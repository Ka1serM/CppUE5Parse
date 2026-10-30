#pragma once

#include "ue5parse/runtime/Engine/Components/LightComponent.h"

#include "DirectionalLightComponent.generated.h"

namespace ue5parse::runtime {

// UDirectionalLightComponent: a light from infinitely far away, such as the sun (UE's
// Components/DirectionalLightComponent.h). Its intensity is in lux.
UCLASS()
class UDirectionalLightComponent : public ULightComponent {
    GENERATED_BODY()
public:
    explicit UDirectionalLightComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float LightSourceAngle = 0.5357f;

    ELightUnits GetLightUnits() const override { return ELightUnits::Unitless; }
};

}
