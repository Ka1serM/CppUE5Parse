#include "Engine/Source/Runtime/Engine/Engine/SkyLight.h"

#include "Engine/Source/Runtime/Engine/Components/SkyLightComponent.h"

namespace ue5parse::runtime {

ASkyLight::ASkyLight(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    LightComponent = ObjectInitializer.CreateDefaultSubobject<USkyLightComponent>(FName("SkyLightComponent0"));
    RootComponent = LightComponent;
    bHidden = false;
}

}
