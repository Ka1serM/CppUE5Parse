#include "Engine/Plugins/Animation/ControlRig/Source/ControlRig/ControlRigControlActor.h"

namespace ue5parse::runtime {

AControlRigControlActor::AControlRigControlActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    ActorRootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComponent0"));
    RootComponent = ActorRootComponent;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
    PrimaryActorTick.bStartWithTickEnabled = true;
    bAllowTickBeforeBeginPlay = true;
}

}
