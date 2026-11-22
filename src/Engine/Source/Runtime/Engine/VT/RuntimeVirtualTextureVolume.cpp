#include "Engine/Source/Runtime/Engine/VT/RuntimeVirtualTextureVolume.h"

#include "Engine/Source/Runtime/Engine/Components/RuntimeVirtualTextureComponent.h"

namespace ue5parse::runtime {

ARuntimeVirtualTextureVolume::ARuntimeVirtualTextureVolume(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    VirtualTextureComponent = ObjectInitializer.CreateDefaultSubobject<URuntimeVirtualTextureComponent>(FName("VirtualTextureComponent"));
    RootComponent = VirtualTextureComponent;
}

}
