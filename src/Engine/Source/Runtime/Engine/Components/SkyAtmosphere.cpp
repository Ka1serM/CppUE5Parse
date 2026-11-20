#include "Engine/Source/Runtime/Engine/Components/SkyAtmosphere.h"

#include "Engine/Source/Runtime/Engine/Components/SkyAtmosphereComponent.h"

namespace ue5parse::runtime {

ASkyAtmosphere::ASkyAtmosphere(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SkyAtmosphereComponent = ObjectInitializer.CreateDefaultSubobject<USkyAtmosphereComponent>(FName("SkyAtmosphereComponent"));
    RootComponent = SkyAtmosphereComponent;
}

}
