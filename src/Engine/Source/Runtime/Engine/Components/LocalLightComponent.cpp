#include "Engine/Source/Runtime/Engine/Components/LocalLightComponent.h"

#include <algorithm>

namespace ue5parse::runtime {

ULocalLightComponent::ULocalLightComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Intensity = 5000;
}

}
