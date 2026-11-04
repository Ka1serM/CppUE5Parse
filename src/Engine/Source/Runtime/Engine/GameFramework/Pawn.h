#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "Pawn.generated.h"

namespace ue5parse::runtime {

class AController;
class UPawnMovementComponent;
class APlayerController;
class APlayerState;

UCLASS()
class APawn : public AActor {
    GENERATED_BODY()
public:
    explicit APawn(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    float BaseEyeHeight = 64.0f;
    UPROPERTY(Replicated)
    uint16 RemoteViewPitch = 0;
    UPROPERTY(ReplicatedUsing = OnRep_PlayerState)
    TObjectPtr<APlayerState> PlayerState = nullptr;
    UPROPERTY(ReplicatedUsing = OnRep_Controller)
    TObjectPtr<AController> Controller = nullptr;

    UFUNCTION(BlueprintCallable)
    AController* GetController() const { return Controller; }
    template <class T>
    T* GetController() const {
        return Cast<T>(Controller);
    }
    APlayerState* GetPlayerState() const { return PlayerState; }
    UFUNCTION(BlueprintCallable)
    virtual UPawnMovementComponent* GetMovementComponent() const;
    FVector GetVelocity() const override;
    UFUNCTION(BlueprintCallable)
    virtual FRotator GetBaseAimRotation() const;
    UFUNCTION(BlueprintCallable)
    virtual FRotator GetControlRotation() const;
    UFUNCTION(BlueprintCallable)
    virtual bool IsLocallyControlled() const;
    UFUNCTION(BlueprintCallable)
    bool IsLocallyViewed() const;
    UFUNCTION(BlueprintCallable)
    virtual bool IsPlayerControlled() const;
    UFUNCTION(BlueprintCallable)
    bool IsControlled() const { return Controller != nullptr; }
    UFUNCTION(BlueprintCallable)
    virtual bool IsBotControlled() const { return false; }
    UFUNCTION(BlueprintCallable)
    APlayerController* GetLocalViewingPlayerController() const { return nullptr; }
    UFUNCTION()
    virtual void OnRep_Controller();
    UFUNCTION()
    virtual void OnRep_PlayerState() { SetPlayerState(PlayerState); }
    void SetPlayerState(APlayerState* NewPlayerState);
    virtual void NotifyControllerChanged();
    UPROPERTY()
    TObjectPtr<AController> PreviousController = nullptr;
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveControllerChanged(AController* OldController, AController* NewController);
    void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const override;
};

}
