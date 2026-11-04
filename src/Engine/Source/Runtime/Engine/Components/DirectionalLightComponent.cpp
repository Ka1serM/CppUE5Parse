#include "Engine/Source/Runtime/Engine/Components/DirectionalLightComponent.h"

namespace ue5parse::runtime {

UDirectionalLightComponent::UDirectionalLightComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Intensity = 10;
}

}
