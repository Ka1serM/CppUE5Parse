#pragma once

#include "ue5parse/runtime/Engine/GameFramework/MovementComponent.h"

#include "ProjectileMovementComponent.generated.h"

namespace ue5parse::runtime {

// UProjectileMovementComponent: the ballistic movement of a projectile (UE's GameFramework/ProjectileMovementComponent.h).
UCLASS()
class UProjectileMovementComponent : public UMovementComponent {
    GENERATED_BODY()
public:
    explicit UProjectileMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
