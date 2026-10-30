#pragma once

#include "ue5parse/runtime/Engine/Components/PointLightComponent.h"

#include "SpotLightComponent.generated.h"

namespace ue5parse::runtime {

// USpotLightComponent: a point light limited to a cone (UE's Components/SpotLightComponent.h).
UCLASS()
class USpotLightComponent : public UPointLightComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    float InnerConeAngle = 0;
    UPROPERTY()
    float OuterConeAngle = 44.0f;

    float ComputeLightBrightness() const override;
    // The clamped outer cone angle, in radians.
    float GetHalfConeAngle() const;
    float GetCosHalfConeAngle() const;
};

}
