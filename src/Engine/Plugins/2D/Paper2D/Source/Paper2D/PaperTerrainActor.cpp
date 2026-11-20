#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperTerrainActor.h"

#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperTerrainComponent.h"
#include "Engine/Plugins/2D/Paper2D/Source/Paper2D/PaperTerrainSplineComponent.h"

namespace ue5parse::runtime {

APaperTerrainActor::APaperTerrainActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DummyRoot = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("RootComponent"));
    SplineComponent = ObjectInitializer.CreateDefaultSubobject<UPaperTerrainSplineComponent>(FName("SplineComponent"));
    RenderComponent = ObjectInitializer.CreateDefaultSubobject<UPaperTerrainComponent>(FName("RenderComponent"));
    SplineComponent->SetupAttachment(DummyRoot);
    RenderComponent->SetupAttachment(DummyRoot);
    RenderComponent->AssociatedSpline = SplineComponent;
    RootComponent = DummyRoot;
}

}
