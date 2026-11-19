#include "Engine/Source/Runtime/Engine/Engine/PointLight.h"

#include "Engine/Source/Runtime/Engine/Components/PointLightComponent.h"

namespace ue5parse::runtime {

APointLight::APointLight(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UPointLightComponent>(FName("LightComponent0"))) {
    PointLightComponent = CastChecked<UPointLightComponent>(LightComponent);
    PointLightComponent->Mobility = EComponentMobility::Stationary;
    RootComponent = PointLightComponent;
}

}
