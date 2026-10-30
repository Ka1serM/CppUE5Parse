#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

#include "Pawn.generated.h"

namespace ue5parse::runtime {

class AController;
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
    UPROPERTY()
    uint8 RemoteViewPitch = 0;
    UPROPERTY()
    APlayerState* PlayerState = nullptr;
    UPROPERTY()
    AController* Controller = nullptr;

    UFUNCTION(BlueprintCallable)
    AController* GetController() const { return Controller; }
    template <class T>
    T* GetController() const {
        return Cast<T>(Controller);
    }
    APlayerState* GetPlayerState() const { return PlayerState; }
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
    UFUNCTION(BlueprintCallable)
    virtual void OnRep_Controller(AController* OldController) {}
    UFUNCTION(BlueprintCallable)
    virtual void OnRep_PlayerState(APlayerState* PreviousPlayerState) {}
    // The eyes' place (UE's GetActorEyesViewPoint): the actor's location raised by BaseEyeHeight, the base aim rotation.
    virtual void GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const;
    void SetRemoteViewPitch(float NewRemoteViewPitch);
};

}
