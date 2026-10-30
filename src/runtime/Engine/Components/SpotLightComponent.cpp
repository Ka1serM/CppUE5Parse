#include "ue5parse/runtime/Engine/Components/SpotLightComponent.h"

#include <algorithm>
#include <cmath>

#include "ue5parse/runtime/RenderCore/RenderUtils.h"

// UE's Components/SpotLightComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

float USpotLightComponent::GetHalfConeAngle() const {
    constexpr float Pi = 3.1415926535897932f;
    const float ClampedInnerConeAngle = std::clamp(InnerConeAngle, 0.0f, 89.0f) * Pi / 180.0f;
    return std::clamp(OuterConeAngle * Pi / 180.0f, ClampedInnerConeAngle + 0.001f, 89.0f * Pi / 180.0f + 0.001f);
}

float USpotLightComponent::GetCosHalfConeAngle() const { return std::cos(GetHalfConeAngle()); }

float USpotLightComponent::ComputeLightBrightness() const {
    constexpr float Pi = 3.1415926535897932f;
    float LightBrightness = ULightComponent::ComputeLightBrightness();
    if (bUseInverseSquaredFalloff) {
        if (IntensityUnits == ELightUnits::Candelas) {
            LightBrightness *= (100.f * 100.f);
        } else if (IntensityUnits == ELightUnits::Lumens) {
            LightBrightness *= (100.f * 100.f / 2.f / Pi / (1.f - GetCosHalfConeAngle()));   // the cone instead of the sphere
        } else if (IntensityUnits == ELightUnits::EV) {
            LightBrightness = EV100ToLuminance(LightBrightness) * (100.f * 100.f);
        } else {
            LightBrightness *= 16;
        }
    }
    return LightBrightness;
}

}
