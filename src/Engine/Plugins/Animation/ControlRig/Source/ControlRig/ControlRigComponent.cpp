#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigComponent.h"

namespace ue5parse::runtime {

UControlRigComponent::UControlRigComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bTickEvenWhenPaused = true;
    bTickInEditor = true;
}

}
