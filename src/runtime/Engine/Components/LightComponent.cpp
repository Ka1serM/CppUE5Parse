#include "ue5parse/runtime/Engine/Components/LightComponent.h"

#include <algorithm>
#include <cmath>

#include "ue5parse/provider/FileProvider.h"
#include "ue5parse/runtime/Engine/Engine/TextureLightProfile.h"
#include "ue5parse/runtime/RenderCore/RenderUtils.h"

// UE's Components/LightComponent.cpp (UE 5.3): ULightComponentBase and ULightComponent.

namespace ue5parse::runtime {

void ULightComponentBase::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.HasGameQuirk(QUIRK_LightComponentPadding)) Ar.SkipBytes(24);
}

void ULightComponent::PostLoad() {
    Super::PostLoad();
    Intensity = std::max(0.0f, Intensity);
}

ELightUnits ULightComponent::GetLightUnits() const { return ELightUnits::Unitless; }

float ULightComponent::ComputeLightBrightness() const {
    float LightBrightness = Intensity;
    if (IESTexture != nullptr) {
        // An EV intensity goes through luminance, so that the IES brightness applies to a linear value.
        const bool bEVUnit = GetLightUnits() == ELightUnits::EV;
        if (bEVUnit) LightBrightness = EV100ToLuminance(Intensity);
        if (bUseIESBrightness) LightBrightness = IESTexture->Brightness * IESBrightnessScale;
        LightBrightness *= IESTexture->TextureMultiplier;
        if (bEVUnit) LightBrightness = LuminanceToEV100(LightBrightness);
    }
    return LightBrightness;
}

FLinearColor ULightComponent::GetColoredLightBrightness() const {
    // The light color is sRGB; the renderer works with linear values in the default working color space.
    FLinearColor Energy = FLinearColor(LightColor) * ComputeLightBrightness();
    if (bUseTemperature) Energy *= FLinearColor::MakeFromColorTemperature(Temperature);
    return Energy;
}

}
