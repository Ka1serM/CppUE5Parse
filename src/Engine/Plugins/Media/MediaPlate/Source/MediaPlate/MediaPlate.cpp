#include "Engine/Plugins/Media/MediaPlate/Source/MediaPlate/MediaPlate.h"

#include "Engine/Plugins/Media/MediaPlate/Source/MediaPlate/MediaPlateComponent.h"
#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

namespace ue5parse::runtime {

AMediaPlate::AMediaPlate(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("Root"));
    MediaPlateComponent = ObjectInitializer.CreateDefaultSubobject<UMediaPlateComponent>(FName("MediaPlateComponent0"));
    StaticMeshComponent = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(FName("StaticMeshComponent"));
    StaticMeshComponent->SetupAttachment(RootComponent);
    StaticMeshComponent->bCastStaticShadow = false;
    StaticMeshComponent->bCastDynamicShadow = false;
    MediaPlateComponent->StaticMeshComponent = StaticMeshComponent;
}

}
