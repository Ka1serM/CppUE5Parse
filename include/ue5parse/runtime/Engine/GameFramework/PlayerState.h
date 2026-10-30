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
    UPROPERTY(ReplicatedUsing = OnRep_PlayerId)
    int32 PlayerId = 0;
    UPROPERTY(Replicated)
    bool bIsABot = false;
    UPROPERTY()
    TObjectPtr<APawn> PawnPrivate = nullptr;
    UPROPERTY(ReplicatedUsing = OnRep_PlayerName)
    FString PlayerNamePrivate;

    UFUNCTION()
    virtual void OnRep_PlayerId() {}
    UFUNCTION()
    virtual void OnRep_PlayerName() {}
    // A controller's player state arrived (UE's APlayerState::ClientInitialize).
    virtual void ClientInitialize(class AController* C);
    void SetPawnPrivate(APawn* InPawn) { PawnPrivate = InPawn; }

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
