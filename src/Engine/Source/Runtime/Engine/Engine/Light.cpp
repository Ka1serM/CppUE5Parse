#include "Engine/Source/Runtime/Engine/Engine/Light.h"

#include "Engine/Source/Runtime/Engine/Components/DirectionalLightComponent.h"
#include "Engine/Source/Runtime/Engine/Components/RectLightComponent.h"
#include "Engine/Source/Runtime/Engine/Components/SpotLightComponent.h"

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
