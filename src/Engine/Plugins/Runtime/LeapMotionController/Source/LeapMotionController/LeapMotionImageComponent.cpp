#include "Engine/Plugins/Runtime/LeapMotionController/Source/LeapMotionController/LeapMotionImageComponent.h"

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

namespace ue5parse::runtime {

ULeapMotionImageComponent::ULeapMotionImageComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DisplaySurfaceComponent = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(FName("DisplaySurfaceComponent"));
}

}
