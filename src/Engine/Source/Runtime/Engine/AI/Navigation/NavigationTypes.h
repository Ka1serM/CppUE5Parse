#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/SoftObjectPtr.h"

#include "NavigationTypes.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FMovementProperties {
    GENERATED_BODY()
    UPROPERTY()
    uint8 bCanCrouch : 1 = 0;
    UPROPERTY()
    uint8 bCanJump : 1 = 0;
    UPROPERTY()
    uint8 bCanWalk : 1 = 0;
    UPROPERTY()
    uint8 bCanSwim : 1 = 0;
    UPROPERTY()
    uint8 bCanFly : 1 = 0;
};

USTRUCT()
struct FNavAgentProperties : public FMovementProperties {
    GENERATED_BODY()
    UPROPERTY()
    float AgentRadius = -1.0f;
    UPROPERTY()
    float AgentHeight = -1.0f;
    UPROPERTY()
    float AgentStepHeight = -1.0f;
    UPROPERTY()
    float NavWalkingSearchHeightScale = 0.5f;
    UPROPERTY()
    FSoftClassPath PreferredNavData;
};

USTRUCT()
struct FNavMovementProperties {
    GENERATED_BODY()
    UPROPERTY()
    float FixedPathBrakingDistance = 0;
    UPROPERTY()
    bool bUpdateNavAgentWithOwnersCollision = true;
    UPROPERTY()
    bool bUseAccelerationForPaths = false;
    UPROPERTY()
    bool bUseFixedBrakingDistanceForPaths = false;
    UPROPERTY()
    bool bStopMovementAbortPaths = true;
};

USTRUCT()
struct FNavLinkAuxiliaryId {
    GENERATED_BODY()
    UPROPERTY()
    uint64 Id = 0;
};

USTRUCT()
struct FNavLinkId {
    GENERATED_BODY()
    UPROPERTY()
    uint64 Id = 0;
};

}
