#include "Engine/Plugins/Media/MediaPlate/Source/MediaPlate/MediaPlateComponent.h"

#include "Engine/Source/Runtime/MediaAssets/MediaPlaylist.h"

namespace ue5parse::runtime {

UMediaPlateComponent::UMediaPlateComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    MediaPlaylist = ObjectInitializer.CreateDefaultSubobject<UMediaPlaylist>(FName("MediaPlaylist0"));
}

}
