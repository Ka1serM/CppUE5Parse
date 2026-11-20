#include "Engine/Source/Runtime/Engine/VectorField/VectorFieldVolume.h"

#include "Engine/Source/Runtime/Engine/Components/VectorFieldComponent.h"

namespace ue5parse::runtime {

AVectorFieldVolume::AVectorFieldVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    VectorFieldComponent = ObjectInitializer.CreateDefaultSubobject<UVectorFieldComponent>(FName("VectorFieldComponent0"));
    RootComponent = VectorFieldComponent;
}

}
