#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Controller.h"

#include "PlayerController.generated.h"

namespace ue5parse::runtime {

UCLASS()
class APlayerController : public AController {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<APawn> SpectatorPawn = nullptr;

    bool IsPlayerController() const override { return true; }
    APawn* GetPawnOrSpectator() const { return GetPawn() != nullptr ? GetPawn() : SpectatorPawn.Get(); }
    UFUNCTION(BlueprintCallable)
    bool InCinematicMode() const { return false; }
};

}
