#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/MovementComponent.h"

#include "ProjectileMovementComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UProjectileMovementComponent : public UMovementComponent {
    GENERATED_BODY()
public:
    explicit UProjectileMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
