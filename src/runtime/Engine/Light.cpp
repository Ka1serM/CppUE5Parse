#include "ue5parse/runtime/Engine/Engine/Light.h"

#include "ue5parse/runtime/Engine/Components/DirectionalLightComponent.h"
#include "ue5parse/runtime/Engine/Components/RectLightComponent.h"
#include "ue5parse/runtime/Engine/Components/SpotLightComponent.h"

// UE's DirectionalLight.cpp, PointLight.cpp, SpotLight.cpp and RectLight.cpp (UE 5.3): the light component as the
// root, with the rotation that a level leaves out where the light keeps it. Only the editor's arrow is left out.

namespace ue5parse::runtime {

namespace {

template <typename T>
T* lightComponent(const FObjectInitializer& ObjectInitializer, ALight& Light, const FRotator Rotation) {
    T* Component = ObjectInitializer.CreateDefaultSubobject<T>(FName("LightComponent0"));
    if (Component != nullptr) {
        Component->Mobility = EComponentMobility::Stationary;
        Component->SetRelativeRotation(Rotation);
    }
    Light.LightComponent = Component;
    Light.RootComponent = Component;
    return Component;
}

}

ADirectionalLight::ADirectionalLight(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    lightComponent<UDirectionalLightComponent>(ObjectInitializer, *this, FRotator(-46.0f, 0.0f, 0.0f));
}

APointLight::APointLight(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PointLightComponent = lightComponent<UPointLightComponent>(ObjectInitializer, *this, FRotator::ZeroRotator);
}

ASpotLight::ASpotLight(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SpotLightComponent = lightComponent<USpotLightComponent>(ObjectInitializer, *this, FRotator(-90.0f, 0.0f, 0.0f));
}

ARectLight::ARectLight(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RectLightComponent = lightComponent<URectLightComponent>(ObjectInitializer, *this, FRotator(-90.0f, 0.0f, 0.0f));
}

}
