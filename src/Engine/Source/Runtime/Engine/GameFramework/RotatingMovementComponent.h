#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/MovementComponent.h"

#include "RotatingMovementComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class URotatingMovementComponent : public UMovementComponent {
    GENERATED_BODY()
public:
    explicit URotatingMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
