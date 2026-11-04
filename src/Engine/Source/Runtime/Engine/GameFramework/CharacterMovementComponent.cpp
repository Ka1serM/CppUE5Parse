#include "Engine/Source/Runtime/Engine/GameFramework/CharacterMovementComponent.h"

#include "Engine/Source/Runtime/Engine/GameFramework/Character.h"

namespace ue5parse::runtime {

namespace {

constexpr uint8 GroundShift = 3;
constexpr uint8 CustomModeThr = 2 * (1 << GroundShift);
constexpr uint8 GroundMask = (1 << GroundShift) - 1;

}

void UCharacterMovementComponent::ApplyNetworkMovementMode(uint8 ReceivedMode) {
    if (ReceivedMode < CustomModeThr) {
        MovementMode = EMovementMode(ReceivedMode & GroundMask);
        CustomMovementMode = 0;
    } else {
        MovementMode = MOVE_Custom;
        CustomMovementMode = ReceivedMode - CustomModeThr;
    }
}

void UCharacterMovementComponent::SetUpdatedComponent(USceneComponent* NewUpdatedComponent) {
    Super::SetUpdatedComponent(NewUpdatedComponent);
    CharacterOwner = Cast<ACharacter>(PawnOwner);
}

}
