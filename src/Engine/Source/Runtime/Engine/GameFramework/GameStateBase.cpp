#include "Engine/Source/Runtime/Engine/GameFramework/GameStateBase.h"

#include "Engine/Source/Runtime/Engine/Engine/World.h"

namespace ue5parse::runtime {

void AGameStateBase::PostInitializeComponents() {
    Super::PostInitializeComponents();
    if (UWorld* World = GetWorld()) World->SetGameState(this);
}

}
