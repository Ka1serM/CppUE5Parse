#include "Engine/Source/Runtime/Engine/Components/StereoLayerComponent.h"

namespace ue5parse::runtime {

UStereoLayerComponent::UStereoLayerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
}

}
