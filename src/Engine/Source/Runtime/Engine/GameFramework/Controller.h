#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "Controller.generated.h"

namespace ue5parse::runtime {

class APawn;
class APlayerState;

UCLASS()
class AController : public AActor {
    GENERATED_BODY()
public:
    explicit AController(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USceneComponent> TransformComponent = nullptr;
    UPROPERTY(ReplicatedUsing = OnRep_PlayerState)
    TObjectPtr<APlayerState> PlayerState = nullptr;
    UPROPERTY(ReplicatedUsing = OnRep_Pawn)
    TObjectPtr<APawn> Pawn = nullptr;
    UPROPERTY()
    FRotator ControlRotation = FRotator::ZeroRotator;

    APawn* OldPawn = nullptr;

    APawn* GetPawn() const { return Pawn; }
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
    void PostInitializeComponents() override;
    void Destroyed() override;
    virtual void GetPlayerViewPoint(FVector& OutLocation, FRotator& OutRotation) const;
};

}
