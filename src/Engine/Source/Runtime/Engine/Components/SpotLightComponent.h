#pragma once

#include "Engine/Source/Runtime/Engine/Components/PointLightComponent.h"

#include "SpotLightComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USpotLightComponent : public UPointLightComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    float InnerConeAngle = 0;
    UPROPERTY()
    float OuterConeAngle = 44.0f;

    float ComputeLightBrightness() const override;
    float GetHalfConeAngle() const;
    float GetCosHalfConeAngle() const;
};

}
