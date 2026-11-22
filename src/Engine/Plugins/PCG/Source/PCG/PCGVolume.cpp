#include "Engine/Plugins/PCG/Source/PCG/PCGVolume.h"

#include "Engine/Plugins/PCG/Source/PCG/PCGComponent.h"

namespace ue5parse::runtime {

APCGVolume::APCGVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PCGComponent = ObjectInitializer.CreateDefaultSubobject<UPCGComponent>(FName("PCG Component"));
}

}
