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
    UPROPERTY()
    APlayerState* PlayerState = nullptr;
    UPROPERTY()
    APawn* Pawn = nullptr;
    UPROPERTY()
    FRotator ControlRotation = FRotator::ZeroRotator;

    APawn* GetPawn() const { return Pawn; }
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
    // The controller joins the world's controllers, and leaves them when destroyed.
    void PostInitializeComponents() override;
    void Destroyed() override;
    // The view the controller's pawn sees (UE's GetPlayerViewPoint): the pawn's eyes.
    virtual void GetPlayerViewPoint(FVector& OutLocation, FRotator& OutRotation) const;
};

}
