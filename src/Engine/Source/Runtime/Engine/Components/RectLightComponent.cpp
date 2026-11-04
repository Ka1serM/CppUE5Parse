#include "Engine/Source/Runtime/Engine/Components/RectLightComponent.h"

#include "Engine/Source/Runtime/RenderCore/RenderUtils.h"

namespace ue5parse::runtime {

float URectLightComponent::ComputeLightBrightness() const {
    constexpr float Pi = 3.1415926535897932f;
    float LightBrightness = Super::ComputeLightBrightness();
    if (IntensityUnits == ELightUnits::Candelas) {
        LightBrightness *= (100.f * 100.f);
    } else if (IntensityUnits == ELightUnits::Lumens) {
        LightBrightness *= (100.f * 100.f / Pi);
    } else if (IntensityUnits == ELightUnits::EV) {
        LightBrightness *= (100.f * 100.f) * EV100ToLuminance(LightBrightness);
    } else {
        LightBrightness *= 16;
    }
    return LightBrightness;
}

}
