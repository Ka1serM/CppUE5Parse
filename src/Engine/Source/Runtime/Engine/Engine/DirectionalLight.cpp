#include "Engine/Source/Runtime/Engine/Engine/DirectionalLight.h"

#include "Engine/Source/Runtime/Engine/Components/DirectionalLightComponent.h"

namespace ue5parse::runtime {

ADirectionalLight::ADirectionalLight(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<UDirectionalLightComponent>(FName("LightComponent0"))) {
    DirectionalLightComponent = CastChecked<UDirectionalLightComponent>(LightComponent);
    DirectionalLightComponent->Mobility = EComponentMobility::Stationary;
    RootComponent = DirectionalLightComponent;
    DirectionalLightComponent->SetRelativeRotation(FRotator(-46.0f, 0.0f, 0.0f));
    DirectionalLightComponent->SetRelativeScale3D(FVector(2.5f, 2.5f, 2.5f));
}

}
