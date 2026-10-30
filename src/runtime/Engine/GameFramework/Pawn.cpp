#include "ue5parse/runtime/Engine/GameFramework/Pawn.h"

#include "ue5parse/runtime/Engine/GameFramework/Character.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/Controller.h"
#include "ue5parse/runtime/Engine/GameFramework/PlayerController.h"

// UE's Pawn.cpp, Character.cpp and Controller.cpp (UE 5.3): what the view and possession natives need.

namespace ue5parse::runtime {

APawn::APawn(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

FRotator APawn::GetBaseAimRotation() const {
    FVector POVLoc;
    FRotator POVRot;
    if (Controller != nullptr) {
        Controller->GetPlayerViewPoint(POVLoc, POVRot);
        return POVRot;
    }
    POVRot = GetActorRotation();
    // A remote pawn: the replicated view pitch, compressed to a byte.
    if (FMath::IsNearlyZero(POVRot.Pitch)) POVRot.Pitch = FRotator::DecompressAxisFromByte(RemoteViewPitch);
    return POVRot;
}

FRotator APawn::GetControlRotation() const { return Controller ? Controller->GetControlRotation() : FRotator::ZeroRotator; }

bool APawn::IsLocallyControlled() const { return Controller != nullptr && Controller->IsLocalController(); }

bool APawn::IsLocallyViewed() const { return IsLocallyControlled(); }

bool APawn::IsPlayerControlled() const { return PlayerState != nullptr && Controller != nullptr && Controller->IsPlayerController(); }

void APawn::GetActorEyesViewPoint(FVector& OutLocation, FRotator& OutRotation) const {
    OutLocation = GetActorLocation() + FVector(0, 0, BaseEyeHeight);
    OutRotation = GetBaseAimRotation();
}

void APawn::SetRemoteViewPitch(float NewRemoteViewPitch) {
    RemoteViewPitch = FRotator::CompressAxisToByte(NewRemoteViewPitch);
}

ACharacter::ACharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

bool AController::IsLocalController() const {
    const ENetMode NetMode = GetNetMode();
    if (NetMode == NM_Standalone) return true;
    if (NetMode == NM_Client && GetLocalRole() == ROLE_AutonomousProxy) return true;
    return GetRemoteRole() != ROLE_AutonomousProxy && GetLocalRole() == ROLE_Authority;
}

void AController::PostInitializeComponents() {
    Super::PostInitializeComponents();
    if (IsGarbage()) return;
    if (UWorld* World = GetWorld()) World->AddController(this);
}

void AController::Destroyed() {
    if (UWorld* World = GetWorld()) World->RemoveController(this);
    Super::Destroyed();
}

bool AController::IsLocalPlayerController() const { return IsPlayerController() && IsLocalController(); }

void AController::GetPlayerViewPoint(FVector& OutLocation, FRotator& OutRotation) const {
    if (Pawn != nullptr) {
        OutLocation = Pawn->GetActorLocation() + FVector(0, 0, Pawn->BaseEyeHeight);
    } else {
        OutLocation = GetActorLocation();
    }
    OutRotation = GetControlRotation();
}

}
