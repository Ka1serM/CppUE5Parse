#pragma once

#include "ue5parse/runtime/Engine/GameFramework/Actor.h"

#include "Controller.generated.h"

namespace ue5parse::runtime {

class APawn;
class APlayerState;

// AController: what possesses a pawn and holds its view rotation (UE's GameFramework/Controller.h).
UCLASS()
class AController : public AActor {
    GENERATED_BODY()
public:
    explicit AController(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    USceneComponent* TransformComponent = nullptr;
    UPROPERTY(ReplicatedUsing = OnRep_PlayerState)
    APlayerState* PlayerState = nullptr;
    UPROPERTY(ReplicatedUsing = OnRep_Pawn)
    APawn* Pawn = nullptr;
    UPROPERTY()
    FRotator ControlRotation = FRotator::ZeroRotator;

    // The pawn before the last OnRep_Pawn (UE's TWeakObjectPtr OldPawn).
    APawn* OldPawn = nullptr;

    APawn* GetPawn() const { return Pawn; }
    // The possessed pawn, and the pawn's controller link (UE's AController::SetPawn and SetPawnFromRep).
    virtual void SetPawn(APawn* InPawn) { Pawn = InPawn; }
    virtual void SetPawnFromRep(APawn* InPawn);
    UFUNCTION()
    virtual void OnRep_Pawn();
    UFUNCTION()
    virtual void OnRep_PlayerState();
    UFUNCTION(BlueprintCallable)
    APawn* K2_GetPawn() const { return Pawn; }
    UFUNCTION(BlueprintCallable)
    virtual FRotator GetControlRotation() const { return ControlRotation; }
    UFUNCTION(BlueprintCallable)
    virtual void SetControlRotation(const FRotator& NewRotation) { ControlRotation = NewRotation; }
    UFUNCTION(BlueprintCallable)
    bool IsLocalController() const;
    UFUNCTION(BlueprintCallable)
    bool IsLocalPlayerController() const;
    virtual bool IsPlayerController() const { return false; }
    UFUNCTION(BlueprintImplementableEvent)
    void ReceivePossess(APawn* PossessedPawn);
    UFUNCTION(BlueprintImplementableEvent)
    void ReceiveUnPossess(APawn* UnpossessedPawn);
    // The controller joins the world's controllers, and leaves them when destroyed.
    void PostInitializeComponents() override;
    void Destroyed() override;
    // The view the controller's pawn sees (UE's GetPlayerViewPoint): the pawn's eyes.
    virtual void GetPlayerViewPoint(FVector& OutLocation, FRotator& OutRotation) const;
};

}
