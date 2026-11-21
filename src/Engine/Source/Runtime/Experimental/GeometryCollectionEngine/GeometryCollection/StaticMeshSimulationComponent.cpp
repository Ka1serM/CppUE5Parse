#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/StaticMeshSimulationComponent.h"

namespace ue5parse::runtime {

UStaticMeshSimulationComponent::UStaticMeshSimulationComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
}

}
