#pragma once

#include "Engine/Source/Runtime/Engine/AI/Navigation/NavigationTypes.h"
#include "Engine/Source/Runtime/Engine/GameFramework/MovementComponent.h"

#include "NavMovementComponent.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UNavMovementComponent : public UMovementComponent {
    GENERATED_BODY()
public:
    explicit UNavMovementComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FNavAgentProperties NavAgentProps;
    UPROPERTY()
    FMovementProperties MovementState;
    UPROPERTY()
    TObjectPtr<UObject> PathFollowingComp = nullptr;
    UPROPERTY()
    float FixedPathBrakingDistance_DEPRECATED = 0.0f;
    UPROPERTY()
    uint8 bUpdateNavAgentWithOwnersCollision_DEPRECATED : 1 = true;
    UPROPERTY()
    uint8 bUseAccelerationForPaths_DEPRECATED : 1 = false;
    UPROPERTY()
    uint8 bUseFixedBrakingDistanceForPaths_DEPRECATED : 1 = false;
    UPROPERTY()
    FNavMovementProperties NavMovementProperties;
};

}
