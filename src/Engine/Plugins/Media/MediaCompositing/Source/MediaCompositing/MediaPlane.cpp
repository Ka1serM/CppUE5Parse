#include "Engine/Plugins/Media/MediaCompositing/Source/MediaCompositing/MediaPlane.h"

#include "Engine/Plugins/Media/MediaCompositing/Source/MediaCompositing/MediaPlaneComponent.h"

namespace ue5parse::runtime {

AMediaPlane::AMediaPlane(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    MediaPlane = ObjectInitializer.CreateDefaultSubobject<UMediaPlaneComponent>(FName("MediaPlaneComponent"));
    RootComponent = MediaPlane;
}

}
