#include "Engine/Source/Developer/MassEntityTestSuite/LWComponentTestFarmPlot.h"

#include "Engine/Source/Runtime/Engine/Components/HierarchicalInstancedStaticMeshComponent.h"

namespace ue5parse::runtime {

ALWFragmentTestFarmPlot::ALWFragmentTestFarmPlot(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
    HarvestIconISMC = ObjectInitializer.CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(FName("HarvestIconISMC"));
    HarvestIconISMC->SetupAttachment(RootComponent);
}

}
