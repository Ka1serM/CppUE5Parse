#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/MovementComponent.h"

#include "InterpToMovementComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UInterpToMovementComponent : public UMovementComponent {
    GENERATED_BODY()
public:
    explicit UInterpToMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
