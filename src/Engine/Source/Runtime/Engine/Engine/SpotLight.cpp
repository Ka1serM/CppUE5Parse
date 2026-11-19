#include "Engine/Source/Runtime/Engine/Engine/SpotLight.h"

#include "Engine/Source/Runtime/Engine/Components/SpotLightComponent.h"

namespace ue5parse::runtime {

ASpotLight::ASpotLight(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<USpotLightComponent>(FName("LightComponent0"))) {
    SpotLightComponent = CastChecked<USpotLightComponent>(LightComponent);
    SpotLightComponent->Mobility = EComponentMobility::Stationary;
    RootComponent = SpotLightComponent;
    SpotLightComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
}

}
