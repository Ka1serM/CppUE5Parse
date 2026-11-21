#include "Engine/Source/Runtime/Engine/Components/ExponentialHeightFogComponent.h"

namespace ue5parse::runtime {

UExponentialHeightFogComponent::UExponentialHeightFogComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    SecondFogData.FogDensity = 0.0f;
}

}
