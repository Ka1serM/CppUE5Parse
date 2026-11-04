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
    float LightSourceAngle = 0.5357f;

    ELightUnits GetLightUnits() const override { return ELightUnits::Unitless; }
};

}
