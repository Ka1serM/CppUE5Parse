#include "Engine/Source/Runtime/Engine/Engine/ExponentialHeightFog.h"

#include "Engine/Source/Runtime/Engine/Components/ExponentialHeightFogComponent.h"

namespace ue5parse::runtime {

AExponentialHeightFog::AExponentialHeightFog(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Component = ObjectInitializer.CreateDefaultSubobject<UExponentialHeightFogComponent>(FName("HeightFogComponent0"));
    RootComponent = Component;
}

}
