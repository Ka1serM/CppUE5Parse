#include "ue5parse/runtime/Engine/Components/PointLightComponent.h"

#include "ue5parse/runtime/RenderCore/RenderUtils.h"

// UE's Components/PointLightComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

void UPointLightComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (!bUseInverseSquaredFalloff) IntensityUnits = ELightUnits::Unitless;
}

float UPointLightComponent::ComputeLightBrightness() const {
    constexpr float Pi = 3.1415926535897932f;
    float LightBrightness = Super::ComputeLightBrightness();
    if (bUseInverseSquaredFalloff) {
        if (IntensityUnits == ELightUnits::Candelas) {
            LightBrightness *= (100.f * 100.f);   // cm² to m²
        } else if (IntensityUnits == ELightUnits::Lumens) {
            LightBrightness *= (100.f * 100.f / 4 / Pi);   // cm² to m², and the sphere of the 1/r² attenuation
        } else if (IntensityUnits == ELightUnits::EV) {
            LightBrightness = EV100ToLuminance(LightBrightness) * (100.f * 100.f);
        } else {
            LightBrightness *= 16;   // the legacy scale
        }
    }
    return LightBrightness;
}

}
