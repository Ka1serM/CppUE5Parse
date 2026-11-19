#include "Engine/Source/Runtime/Engine/Components/PlanarReflectionComponent.h"

namespace ue5parse::runtime {

UPlanarReflectionComponent::UPlanarReflectionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_DuringPhysics;
    PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
    bTickInEditor = true;
}

}
