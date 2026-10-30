#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Info.h"

#include "PlayerState.generated.h"

namespace ue5parse::runtime {

class APawn;

// APlayerState: the replicated state of one player (UE's GameFramework/PlayerState.h).
UCLASS()
class APlayerState : public AInfo {
    GENERATED_BODY()
public:
    UPROPERTY()
    int32 PlayerId = 0;
    UPROPERTY()
    bool bIsABot = false;
    UPROPERTY()
    APawn* PawnPrivate = nullptr;
    UPROPERTY()
    FString PlayerNamePrivate;

    UFUNCTION(BlueprintCallable)
    int32 GetPlayerId() const { return PlayerId; }
    UFUNCTION(BlueprintCallable)
    bool IsABot() const { return bIsABot; }
    UFUNCTION(BlueprintCallable)
    APawn* GetPawn() const { return PawnPrivate; }
    UFUNCTION(BlueprintCallable)
    FString GetPlayerName() const { return PlayerNamePrivate; }
};

}
