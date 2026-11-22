#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "ChaosNotifyHandlerInterface.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FChaosPhysicsCollisionInfo {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UPrimitiveComponent> Component = nullptr;
    UPROPERTY()
    TObjectPtr<UPrimitiveComponent> OtherComponent = nullptr;
    UPROPERTY()
    FVector Location;
    UPROPERTY()
    FVector Normal;
    UPROPERTY()
    FVector AccumulatedImpulse;
    UPROPERTY()
    FVector Velocity;
    UPROPERTY()
    FVector OtherVelocity;
    UPROPERTY()
    FVector AngularVelocity;
    UPROPERTY()
    FVector OtherAngularVelocity;
    UPROPERTY()
    float Mass = 0.0f;
    UPROPERTY()
    float OtherMass = 0.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChaosPhysicsCollision, const FChaosPhysicsCollisionInfo&, CollisionInfo);

}
