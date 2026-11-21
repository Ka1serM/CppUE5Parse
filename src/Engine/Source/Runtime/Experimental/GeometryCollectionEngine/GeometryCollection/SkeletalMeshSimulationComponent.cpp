#include "Engine/Source/Runtime/Experimental/GeometryCollectionEngine/GeometryCollection/SkeletalMeshSimulationComponent.h"

namespace ue5parse::runtime {

USkeletalMeshSimulationComponent::USkeletalMeshSimulationComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
}

}
