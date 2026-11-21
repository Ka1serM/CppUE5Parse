#include "Engine/Plugins/PCG/Source/PCG/PCGWorldActor.h"

#include "Engine/Plugins/PCG/Source/PCG/Grid/PCGLandscapeCache.h"

namespace ue5parse::runtime {

APCGWorldActor::APCGWorldActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PartitionGridSize = 25600;
    LandscapeCacheObject = ObjectInitializer.CreateDefaultSubobject<UPCGLandscapeCache>(FName("LandscapeCache"));
}

}
