#include "ue5parse/runtime/Engine/GameFramework/CharacterMovementComponent.h"

// UE's Components/CharacterMovementComponent.cpp (UE 5.3): what a simulated proxy applies.

namespace ue5parse::runtime {

namespace {

// PackedMovementModeConstants: the ground mode bit sits above the movement modes, the custom modes above both.
constexpr uint8 GroundShift = 3;
constexpr uint8 CustomModeThr = 2 * (1 << GroundShift);
constexpr uint8 GroundMask = (1 << GroundShift) - 1;

}

void UCharacterMovementComponent::ApplyNetworkMovementMode(uint8 ReceivedMode) {
    // UnpackNetworkMovementMode, then SetMovementMode.
    if (ReceivedMode < CustomModeThr) {
        MovementMode = EMovementMode(ReceivedMode & GroundMask);
        CustomMovementMode = 0;
    } else {
        MovementMode = MOVE_Custom;
        CustomMovementMode = ReceivedMode - CustomModeThr;
    }
}

}
