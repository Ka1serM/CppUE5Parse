#include "Engine/Source/Runtime/HeadMountedDisplay/MotionControllerComponent.h"

namespace ue5parse::runtime {

UMotionControllerComponent::UMotionControllerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = true;
    bAutoActivate = true;
    bWantsInitializeComponent = true;
}

}
