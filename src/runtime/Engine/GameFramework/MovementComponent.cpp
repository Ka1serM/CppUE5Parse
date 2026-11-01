#include "ue5parse/runtime/Engine/GameFramework/MovementComponent.h"

#include "ue5parse/runtime/Engine/GameFramework/Actor.h"
#include "ue5parse/runtime/Engine/GameFramework/Pawn.h"
#include "ue5parse/runtime/Engine/GameFramework/PawnMovementComponent.h"

// UE's Components/MovementComponent.cpp and PawnMovementComponent.cpp (UE 5.3): the updated component and its pawn.

namespace ue5parse::runtime {

void UMovementComponent::InitializeComponent() {
    Super::InitializeComponent();
    SetUpdatedComponent(UpdatedComponent);
}

void UMovementComponent::SetUpdatedComponent(USceneComponent* NewUpdatedComponent) { UpdatedComponent = NewUpdatedComponent; }

void UPawnMovementComponent::SetUpdatedComponent(USceneComponent* NewUpdatedComponent) {
    Super::SetUpdatedComponent(NewUpdatedComponent);
    PawnOwner = NewUpdatedComponent != nullptr ? Cast<APawn>(NewUpdatedComponent->GetOwner()) : nullptr;
}

}
