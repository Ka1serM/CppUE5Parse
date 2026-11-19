#include "Engine/Source/Runtime/Engine/Components/SkyLightComponent.h"

namespace ue5parse::runtime {

USkyLightComponent::USkyLightComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Brightness_DEPRECATED = 1;
    Intensity = 1;
    Mobility = EComponentMobility::Stationary;
    bCastVolumetricShadow = true;
    bCastRaytracedShadow_DEPRECATED = false;
    SamplesPerPixel = 4;
}

}
