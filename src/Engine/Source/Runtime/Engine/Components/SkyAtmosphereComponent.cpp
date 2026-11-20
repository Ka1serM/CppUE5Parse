#include "Engine/Source/Runtime/Engine/Components/SkyAtmosphereComponent.h"

namespace ue5parse::runtime {

void USkyAtmosphereComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    Ar << bStaticLightingBuiltGUID;
}

}
