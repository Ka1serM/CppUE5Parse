#include "ue5parse/runtime/Engine/Components/DirectionalLightComponent.h"

// UE's Components/DirectionalLightComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

UDirectionalLightComponent::UDirectionalLightComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Intensity = 10;
}

}
