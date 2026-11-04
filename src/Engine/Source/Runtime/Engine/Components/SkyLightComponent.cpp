#include "Engine/Source/Runtime/Engine/Components/SkyLightComponent.h"

namespace ue5parse::runtime {

USkyLightComponent::USkyLightComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Intensity = 1;
}

}
