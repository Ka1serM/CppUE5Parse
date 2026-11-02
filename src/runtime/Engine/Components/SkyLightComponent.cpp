#include "ue5parse/runtime/Engine/Components/SkyLightComponent.h"

// UE's Components/SkyLightComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

USkyLightComponent::USkyLightComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Intensity = 1;
}

}
