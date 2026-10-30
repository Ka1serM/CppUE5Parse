#pragma once

#include "ue5parse/runtime/Engine/GameFramework/MovementComponent.h"

#include "NavMovementComponent.generated.h"

namespace ue5parse::runtime {

// UNavMovementComponent: a movement component that navigation can drive (UE's GameFramework/NavMovementComponent.h).
UCLASS(Abstract)
class UNavMovementComponent : public UMovementComponent {
    GENERATED_BODY()
public:
    explicit UNavMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
