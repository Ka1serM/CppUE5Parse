#include "Engine/Source/Runtime/Engine/GameFramework/MovementComponent.h"

#include "Engine/Source/Runtime/Engine/Engine/World.h"
#include "Engine/Source/Runtime/Engine/EngineNativeMissing.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Pawn.h"
#include "Engine/Source/Runtime/Engine/GameFramework/PawnMovementComponent.h"
#include "Engine/Source/Runtime/Engine/GameFramework/WorldSettings.h"

namespace ue5parse::runtime {

void UMovementComponent::OnRegister() {
    Super::OnRegister();
    AutoRegisterUpdatedComponent();
}

void UMovementComponent::InitializeComponent() {
    Super::InitializeComponent();
    AutoRegisterUpdatedComponent();
    SetUpdatedComponent(UpdatedComponent);
}

void UMovementComponent::AutoRegisterUpdatedComponent() {
    if (UpdatedComponent != nullptr || !bAutoRegisterUpdatedComponent)
        return;
    const AActor* MyActor = GetOwner();
    if (MyActor == nullptr)
        return;
    if (USceneComponent* NewUpdatedComponent = MyActor->GetRootComponent())
        SetUpdatedComponent(NewUpdatedComponent);
}

void UMovementComponent::SetUpdatedComponent(USceneComponent* NewUpdatedComponent) {
    UpdatedComponent = NewUpdatedComponent;
}

bool UMovementComponent::ShouldSkipUpdate(float) const {
    return UpdatedComponent == nullptr || UpdatedComponent->Mobility != EComponentMobility::Movable;
}

void UMovementComponent::UpdateComponentVelocity() {
    if (UpdatedComponent != nullptr)
        UpdatedComponent->ComponentVelocity = Velocity;
}

bool UMovementComponent::CheckStillInWorld() {
    if (UpdatedComponent == nullptr)
        return false;
    const UWorld* World = GetWorld();
    if (World == nullptr)
        return false;
    const AWorldSettings* WorldSettings = World->GetWorldSettings();
    //! A world that CreateWorld makes has no settings, and so no world bounds.
    if (WorldSettings == nullptr || !WorldSettings->bEnableWorldBoundsChecks)
        return true;
    AActor* ActorOwner = UpdatedComponent->GetOwner();
    if (!IsValid(ActorOwner))
        return false;
    if (ActorOwner->GetActorLocation().Z >= WorldSettings->KillZ)
        return true;
    UClass* DamageType = WorldSettings->KillZDamageType.Get();
    if (DamageType == nullptr)
        DamageType = UDamageType::StaticClass();
    ActorOwner->FellOutOfWorld(*Cast<UDamageType>(DamageType->GetDefaultObject()));
    return false;
}

void UPawnMovementComponent::SetUpdatedComponent(USceneComponent* NewUpdatedComponent) {
    Super::SetUpdatedComponent(NewUpdatedComponent);
    PawnOwner = NewUpdatedComponent != nullptr ? Cast<APawn>(NewUpdatedComponent->GetOwner()) : nullptr;
}

}
