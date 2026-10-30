#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Controller.h"

#include "PlayerController.generated.h"

namespace ue5parse::runtime {

// APlayerController: the controller of a human player (UE's GameFramework/PlayerController.h).
UCLASS()
class APlayerController : public AController {
    GENERATED_BODY()
public:
    // The pawn a spectating player views from (UE's ASpectatorPawn).
    UPROPERTY()
    APawn* SpectatorPawn = nullptr;

    bool IsPlayerController() const override { return true; }
    APawn* GetSpectatorPawn() const { return SpectatorPawn; }
    APawn* GetPawnOrSpectator() const { return GetPawn() != nullptr ? GetPawn() : SpectatorPawn; }
    UFUNCTION(BlueprintCallable)
    bool InCinematicMode() const { return false; }
};

}
