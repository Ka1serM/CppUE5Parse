#include "Engine/Source/Runtime/Engine/Components/VolumetricCloud.h"

#include "Engine/Source/Runtime/Engine/Components/VolumetricCloudComponent.h"

namespace ue5parse::runtime {

AVolumetricCloud::AVolumetricCloud(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    VolumetricCloudComponent = ObjectInitializer.CreateDefaultSubobject<UVolumetricCloudComponent>(FName("VolumetricCloudComponent"));
    RootComponent = VolumetricCloudComponent;
}

}
