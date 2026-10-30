#include "ue5parse/runtime/Engine/Components/RectLightComponent.h"

#include "ue5parse/runtime/RenderCore/RenderUtils.h"

// UE's Components/RectLightComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

float URectLightComponent::ComputeLightBrightness() const {
    constexpr float Pi = 3.1415926535897932f;
    float LightBrightness = Super::ComputeLightBrightness();
    if (IntensityUnits == ELightUnits::Candelas) {
        LightBrightness *= (100.f * 100.f);
    } else if (IntensityUnits == ELightUnits::Lumens) {
        LightBrightness *= (100.f * 100.f / Pi);   // cm² to m², and the cosine distribution
    } else if (IntensityUnits == ELightUnits::EV) {
        // UE 5.3 multiplies by the intensity once more; 5.6 fixed it.
        LightBrightness *= (100.f * 100.f) * EV100ToLuminance(LightBrightness);
    } else {
        LightBrightness *= 16;
    }
    return LightBrightness;
}

}
