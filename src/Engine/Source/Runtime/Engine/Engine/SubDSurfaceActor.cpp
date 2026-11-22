#include "Engine/Source/Runtime/Engine/Engine/SubDSurfaceActor.h"

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"
#include "Engine/Source/Runtime/Engine/Components/SubDSurfaceComponent.h"

namespace ue5parse::runtime {

ASubDSurfaceActor::ASubDSurfaceActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DisplayMeshComponent = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(FName("DisplayMeshComponent"));
    RootComponent = DisplayMeshComponent;
    SubDSurface = ObjectInitializer.CreateDefaultSubobject<USubDSurfaceComponent>(FName("NewSubDSurfaceComponent"));
    SubDSurface->SetupAttachment(DisplayMeshComponent);
}

}
