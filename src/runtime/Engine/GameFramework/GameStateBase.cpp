#include "ue5parse/runtime/Engine/GameFramework/GameStateBase.h"

#include "ue5parse/runtime/Engine/Engine/World.h"

namespace ue5parse::runtime {

void AGameStateBase::PostInitializeComponents() {
    Super::PostInitializeComponents();
    if (UWorld* World = GetWorld()) World->SetGameState(this);
}

}
