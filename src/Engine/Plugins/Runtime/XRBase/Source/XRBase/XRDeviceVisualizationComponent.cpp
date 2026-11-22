#include "Engine/Plugins/Runtime/XRBase/Source/XRBase/XRDeviceVisualizationComponent.h"

namespace ue5parse::runtime {

UXRDeviceVisualizationComponent::UXRDeviceVisualizationComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    bAutoActivate = true;
    bWantsInitializeComponent = true;
}

}
