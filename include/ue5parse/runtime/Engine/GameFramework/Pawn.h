#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

#include "Pawn.generated.h"

namespace ue5parse::runtime {

class AController;
class UPawnMovementComponent;
class APlayerController;
class APlayerState;

// APawn: an actor a controller possesses (UE's GameFramework/Pawn.h).
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
    APlayerState* PlayerState = nullptr;
    UPROPERTY(ReplicatedUsing = OnRep_Controller)
    AController* Controller = nullptr;

    UFUNCTION(BlueprintCallable)
    AController* GetController() const { return Controller; }
    template <class T>
    T* GetController() const {
        return Cast<T>(Controller);
    }
    APlayerState* GetPlayerState() const { return PlayerState; }
    // The pawn's movement component (UE's APawn::GetMovementComponent).
    UFUNCTION(BlueprintCallable)
    virtual UPawnMovementComponent* GetMovementComponent() const;
    // The movement component's velocity, or the root's when it simulates physics (UE's APawn::GetVelocity).
    FVector GetVelocity() const override;
    // The view rotation: the controller's, else the actor's with the replicated pitch (UE's GetBaseAimRotation).
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
    // The player state, and the state's pawn (UE's APawn::SetPlayerState).
    void SetPlayerState(APlayerState* NewPlayerState);
    // A new controller: ReceiveControllerChanged (UE's APawn::NotifyControllerChanged).
    virtual void NotifyControllerChanged();
    UPROPERTY()
    AController* PreviousController = nullptr;
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveControllerChanged(AController* OldController, AController* NewController);
    // The eyes' place (UE's GetActorEyesViewPoint): the actor's location raised by BaseEyeHeight, the base aim rotation.
    virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const;
    void SetRemoteViewPitch(float NewRemoteViewPitch);
};

}
