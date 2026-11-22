#include "Engine/Source/Developer/MassEntityTestSuite/MassEntityTestFarmPlot.h"

#include "Engine/Source/Runtime/Engine/Components/HierarchicalInstancedStaticMeshComponent.h"

namespace ue5parse::runtime {

AMassEntityTestFarmPlot::AMassEntityTestFarmPlot(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
    HarvestIconISMC = ObjectInitializer.CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(FName("HarvestIconISMC"));
    HarvestIconISMC->SetupAttachment(RootComponent);
}

}
