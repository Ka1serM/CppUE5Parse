#include "Engine/Source/Runtime/Engine/Components/LightComponent.h"

#include <algorithm>
#include <cmath>

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Engine/Engine/TextureLightProfile.h"
#include "Engine/Source/Runtime/RenderCore/RenderUtils.h"

namespace ue5parse::runtime {

void ULightComponentBase::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.GetGame() == EGame::GAME_Valorant)
        Ar.SkipBytes(24);
}

void ULightComponent::PostLoad() {
    Super::PostLoad();
    Intensity = std::max(0.0f, Intensity);
}

ELightUnits ULightComponent::GetLightUnits() const {
    return ELightUnits::Unitless;
}

float ULightComponent::ComputeLightBrightness() const {
    float LightBrightness = Intensity;
    if (IESTexture != nullptr) {
        const bool bEVUnit = GetLightUnits() == ELightUnits::EV;
        if (bEVUnit)
            LightBrightness = EV100ToLuminance(Intensity);
        if (bUseIESBrightness)
            LightBrightness = IESTexture->Brightness * IESBrightnessScale;
        LightBrightness *= IESTexture->TextureMultiplier;
        if (bEVUnit)
            LightBrightness = LuminanceToEV100(LightBrightness);
    }
    return LightBrightness;
}

}
