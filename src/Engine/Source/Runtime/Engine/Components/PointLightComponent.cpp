#include "Engine/Source/Runtime/Engine/Components/PointLightComponent.h"

#include "Engine/Source/Runtime/RenderCore/RenderUtils.h"

namespace ue5parse::runtime {

void UPointLightComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (!bUseInverseSquaredFalloff)
        IntensityUnits = ELightUnits::Unitless;
}

float UPointLightComponent::ComputeLightBrightness() const {
    constexpr float Pi = 3.1415926535897932f;
    float LightBrightness = Super::ComputeLightBrightness();
    if (bUseInverseSquaredFalloff) {
        if (IntensityUnits == ELightUnits::Candelas)
            LightBrightness *= (100.f * 100.f);
        else if (IntensityUnits == ELightUnits::Lumens)
            LightBrightness *= (100.f * 100.f / 4 / Pi);
        else if (IntensityUnits == ELightUnits::EV)
            LightBrightness = EV100ToLuminance(LightBrightness) * (100.f * 100.f);
        else
            LightBrightness *= 16;
    }
    return LightBrightness;
}

}
