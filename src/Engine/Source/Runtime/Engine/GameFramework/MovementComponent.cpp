#include "Engine/Source/Runtime/Engine/GameFramework/MovementComponent.h"

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Pawn.h"
#include "Engine/Source/Runtime/Engine/GameFramework/PawnMovementComponent.h"

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
