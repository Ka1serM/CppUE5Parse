#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Info.h"

#include "GameStateBase.generated.h"

namespace ue5parse::runtime {

class APlayerState;

// AGameStateBase: the replicated state of the game; it is the world's game state once its components are initialized
// (UE's GameFramework/GameStateBase.h).
UCLASS()
class AGameStateBase : public AInfo {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<APlayerState*> PlayerArray;

    void PostInitializeComponents() override;
};

}
