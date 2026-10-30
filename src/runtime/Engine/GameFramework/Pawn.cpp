#include "ue5parse/runtime/Engine/GameFramework/Pawn.h"

#include "ue5parse/runtime/Engine/GameFramework/Character.h"
#include "ue5parse/runtime/Engine/GameFramework/CharacterMovementComponent.h"
#include "ue5parse/runtime/Engine/Engine/World.h"
#include "ue5parse/runtime/Engine/GameFramework/Controller.h"
#include "ue5parse/runtime/Engine/GameFramework/PlayerController.h"
#include "ue5parse/runtime/Engine/GameFramework/PlayerState.h"
#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"
#include "ue5parse/runtime/Engine/Components/SkeletalMeshComponent.h"

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

// The transform component as the root.
AController::AController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    TransformComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("TransformComponent0"));
    RootComponent = TransformComponent;
}

FName ACharacter::MeshComponentName("CharacterMesh0");
FName ACharacter::CharacterMovementComponentName("CharMoveComp");
FName ACharacter::CapsuleComponentName("CollisionCylinder");

// The default subobjects: the capsule as the root, the movement that moves it, and the mesh on it. Only the editor's
// arrow and the collision setup are left out. A game whose engine has a capsule class of its own for characters
// (VALORANT's CharacterCapsuleComponent) makes that one.
ACharacter::ACharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    const char* CapsuleClass = FindNativeClass(FName("CharacterCapsuleComponent")) != nullptr ? "CharacterCapsuleComponent" : "CapsuleComponent";
    auto* Capsule = Cast<USceneComponent>(ObjectInitializer.CreateDefaultSubobject(CapsuleComponentName, CapsuleClass));
    CapsuleComponent = reinterpret_cast<UCapsuleComponent*>(Capsule);
    if (Capsule != nullptr) {
        SetPropertyValue(Capsule, FName("CapsuleRadius"), 34.0f);
        SetPropertyValue(Capsule, FName("CapsuleHalfHeight"), 88.0f);
    }
    RootComponent = Capsule;

    CharacterMovement = ObjectInitializer.CreateDefaultSubobject<UCharacterMovementComponent>(CharacterMovementComponentName);
    if (CharacterMovement != nullptr) CharacterMovement->UpdatedComponent = Capsule;

    Mesh = ObjectInitializer.CreateOptionalDefaultSubobject<USkeletalMeshComponent>(MeshComponentName);
    if (Mesh != nullptr) Mesh->SetupAttachment(Capsule);
}

UPawnMovementComponent* ACharacter::GetMovementComponent() const { return CharacterMovement; }

void ACharacter::PostNetReceive() {
    if (Role == ROLE_SimulatedProxy && CharacterMovement != nullptr) CharacterMovement->ApplyNetworkMovementMode(ReplicatedMovementMode);
    Super::PostNetReceive();
}

UPawnMovementComponent* APawn::GetMovementComponent() const { return FindComponentByClass<UPawnMovementComponent>(); }

// The runtime simulates no physics, so the movement component's velocity is the pawn's.
FVector APawn::GetVelocity() const {
    const UPawnMovementComponent* MovementComponent = GetMovementComponent();
    return MovementComponent != nullptr ? MovementComponent->Velocity : FVector::ZeroVector;
}

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

void APawn::OnRep_Controller() {
    bool bNotifyControllerChange = Controller == nullptr;
    // APawn::Controller can arrive before AController::Pawn: the controller then learns its pawn here, so that its
    // OnRep_Pawn runs.
    if (Controller != nullptr && Controller->GetPawn() == nullptr) {
        Controller->SetPawnFromRep(this);
        bNotifyControllerChange = true;
    }
    if (bNotifyControllerChange) NotifyControllerChanged();
}

void APawn::SetPlayerState(APlayerState* NewPlayerState) {
    if (PlayerState != nullptr && PlayerState->GetPawn() == this) PlayerState->SetPawnPrivate(nullptr);
    PlayerState = NewPlayerState;
    if (PlayerState != nullptr) PlayerState->SetPawnPrivate(this);
}

void APawn::NotifyControllerChanged() {
    ReceiveControllerChanged(PreviousController, Controller);
    PreviousController = Controller;
}

void AController::SetPawnFromRep(APawn* InPawn) {
    Pawn = InPawn;
    OnRep_Pawn();
}

void AController::OnRep_Pawn() {
    APawn* StrongOldPawn = OldPawn;
    // The old pawn does not hear from this controller any more.
    if (StrongOldPawn != nullptr && Pawn != StrongOldPawn && StrongOldPawn->Controller == this) StrongOldPawn->Controller = nullptr;
    OldPawn = Pawn;
    SetPawn(Pawn);
}

void AController::OnRep_PlayerState() {
    if (PlayerState != nullptr) PlayerState->ClientInitialize(this);
}

void APlayerState::ClientInitialize(AController* C) { SetOwner(C); }

}
