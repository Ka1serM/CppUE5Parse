#include "ue5parse/runtime/Engine/Components/LocalLightComponent.h"

#include <algorithm>

// UE's Components/LocalLightComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

ULocalLightComponent::ULocalLightComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Intensity = 5000;
}

float ULocalLightComponent::GetUnitsConversionFactor(ELightUnits SrcUnits, ELightUnits TargetUnits, float CosHalfConeAngle) {
    constexpr float Pi = 3.1415926535897932f;
    CosHalfConeAngle = std::clamp(CosHalfConeAngle, -1.0f, 1.0f - 1.e-4f);
    if (SrcUnits == TargetUnits) return 1.0f;
    // UE's units are centimeters, the SI units meters; EV assumes an implicit 1 m² surface.
    float CnvFactor = 1.0f;
    if (SrcUnits == ELightUnits::Candelas) {
        CnvFactor = 100.f * 100.f;
    } else if (SrcUnits == ELightUnits::Lumens) {
        CnvFactor = 100.f * 100.f / 2.f / Pi / (1.f - CosHalfConeAngle);
    } else if (SrcUnits == ELightUnits::EV) {
        CnvFactor = 100.f * 100.f;
    } else {
        CnvFactor = 16.f;
    }
    if (TargetUnits == ELightUnits::Candelas) {
        CnvFactor *= 1.f / 100.f / 100.f;
    } else if (TargetUnits == ELightUnits::Lumens) {
        CnvFactor *= 2.f * Pi * (1.f - CosHalfConeAngle) / 100.f / 100.f;
    } else if (TargetUnits == ELightUnits::EV) {
        CnvFactor *= 1.f / 100.f / 100.f;
    } else {
        CnvFactor *= 1.f / 16.f;
    }
    return CnvFactor;
}

}
