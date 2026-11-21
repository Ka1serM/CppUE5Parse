#include "Engine/Source/Runtime/Experimental/ChaosSolverEngine/Chaos/ChaosSolverActor.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Experimental/ChaosSolverEngine/Chaos/ChaosGameplayEventDispatcher.h"

namespace ue5parse::runtime {

AChaosSolverActor::AChaosSolverActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryActorTick.TickGroup = TG_PostPhysics;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("SceneComp"));
    RootComponent->Mobility = EComponentMobility::Static;
    if (GetRuntimeProvider().getGame() >= EGame::GAME_UE4_23)
        GameplayEventDispatcherComponent = ObjectInitializer.CreateDefaultSubobject<UChaosGameplayEventDispatcher>(FName("GameplayEventDispatcher"));
}

}
