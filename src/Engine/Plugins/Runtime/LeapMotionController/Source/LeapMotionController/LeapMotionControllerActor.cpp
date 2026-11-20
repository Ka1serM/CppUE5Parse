#include "Engine/Plugins/Runtime/LeapMotionController/Source/LeapMotionController/LeapMotionControllerActor.h"

#include "Engine/Plugins/Runtime/LeapMotionController/Source/LeapMotionController/LeapMotionControllerComponent.h"
#include "Engine/Plugins/Runtime/LeapMotionController/Source/LeapMotionController/LeapMotionImageComponent.h"

namespace ue5parse::runtime {

ALeapMotionControllerActor::ALeapMotionControllerActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    LeapMotionControllerComponent = ObjectInitializer.CreateDefaultSubobject<ULeapMotionControllerComponent>(FName("RootLeapMotionControllerComponent"));
    RootComponent = LeapMotionControllerComponent;
    LeapMotionImageComponent = ObjectInitializer.CreateDefaultSubobject<ULeapMotionImageComponent>(FName("LeapMotionImageComponent"));
}

}
