#pragma once

#include "Engine/Source/Runtime/PhysicsCore/Chaos/ChaosEngineInterface.h"

#include "ConstraintTypes.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FConstraintBaseParams {
    GENERATED_BODY()
    UPROPERTY()
    float Stiffness = 0.0f;
    UPROPERTY()
    float Damping = 0.0f;
    UPROPERTY()
    float Restitution = 0.0f;
    UPROPERTY()
    float ContactDistance = 0.0f;
    UPROPERTY()
    bool bSoftConstraint = false;
};

USTRUCT()
struct FLinearConstraint : public FConstraintBaseParams {
    GENERATED_BODY()
    UPROPERTY()
    float Limit = 0.0f;
    UPROPERTY()
    TEnumAsByte<ELinearConstraintMotion> XMotion = LCM_Locked;
    UPROPERTY()
    TEnumAsByte<ELinearConstraintMotion> YMotion = LCM_Locked;
    UPROPERTY()
    TEnumAsByte<ELinearConstraintMotion> ZMotion = LCM_Locked;
};

USTRUCT()
struct FConeConstraint : public FConstraintBaseParams {
    GENERATED_BODY()
    UPROPERTY()
    float Swing1LimitDegrees = 45.0f;
    UPROPERTY()
    float Swing2LimitDegrees = 45.0f;
    UPROPERTY()
    TEnumAsByte<EAngularConstraintMotion> Swing1Motion = ACM_Free;
    UPROPERTY()
    TEnumAsByte<EAngularConstraintMotion> Swing2Motion = ACM_Free;
};

USTRUCT()
struct FTwistConstraint : public FConstraintBaseParams {
    GENERATED_BODY()
    UPROPERTY()
    float TwistLimitDegrees = 45.0f;
    UPROPERTY()
    TEnumAsByte<EAngularConstraintMotion> TwistMotion = ACM_Free;
};

}
