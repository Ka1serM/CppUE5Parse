#include "Engine/Plugins/Runtime/LeapMotionController/Source/LeapMotionController/LeapMotionHandActor.h"

namespace ue5parse::runtime {

ALeapMotionHandActor::ALeapMotionHandActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("RootSceneComponent"));
}

}
