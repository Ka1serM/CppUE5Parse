#include "Engine/Source/Runtime/Engine/Components/LightmassPortalComponent.h"

namespace ue5parse::runtime {

ULightmassPortalComponent::ULightmassPortalComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Mobility = EComponentMobility::Static;
}

}
