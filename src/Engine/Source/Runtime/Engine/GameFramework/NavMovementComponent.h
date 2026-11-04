#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/MovementComponent.h"

#include "NavMovementComponent.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UNavMovementComponent : public UMovementComponent {
    GENERATED_BODY()
public:
    explicit UNavMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
