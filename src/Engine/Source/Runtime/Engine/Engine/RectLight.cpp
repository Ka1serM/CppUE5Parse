#include "Engine/Source/Runtime/Engine/Engine/RectLight.h"

#include "Engine/Source/Runtime/Engine/Components/RectLightComponent.h"

namespace ue5parse::runtime {

ARectLight::ARectLight(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<URectLightComponent>(FName("LightComponent0"))) {
    RectLightComponent = CastChecked<URectLightComponent>(LightComponent);
    RectLightComponent->Mobility = EComponentMobility::Stationary;
    RootComponent = RectLightComponent;
}

}
