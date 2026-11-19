#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ConstraintDrives.generated.h"

namespace ue5parse::runtime {

namespace EAngularDriveMode {

UENUM()
enum Type : int32 { SLERP, TwistAndSwing };

}

UENUM()
enum class EAngularDriveLimitViolationResponse : uint8 { None, ClampLinear, ClampExact, WidenLimits };

USTRUCT()
struct FConstraintDrive {
    GENERATED_BODY()
    UPROPERTY()
    float Stiffness = 50.0f;
    UPROPERTY()
    float Damping = 1.0f;
    UPROPERTY()
    float MaxForce = 0.0f;
    UPROPERTY()
    bool bEnablePositionDrive = false;
    UPROPERTY()
    bool bEnableVelocityDrive = false;
};

USTRUCT()
struct FLinearDriveConstraint {
    GENERATED_BODY()
    UPROPERTY()
    FVector PositionTarget = FVector::ZeroVector;
    UPROPERTY()
    FVector VelocityTarget = FVector::ZeroVector;
    UPROPERTY()
    FConstraintDrive XDrive;
    UPROPERTY()
    FConstraintDrive YDrive;
    UPROPERTY()
    FConstraintDrive ZDrive;
    UPROPERTY()
    bool bEnablePositionDrive = false;
    UPROPERTY()
    bool bAccelerationMode = false;
};

USTRUCT()
struct FAngularDriveConstraint {
    GENERATED_BODY()
    UPROPERTY()
    FConstraintDrive TwistDrive;
    UPROPERTY()
    FConstraintDrive SwingDrive;
    UPROPERTY()
    FConstraintDrive SlerpDrive;
    UPROPERTY()
    FRotator OrientationTarget = FRotator::ZeroRotator;
    UPROPERTY()
    FVector AngularVelocityTarget = FVector::ZeroVector;
    UPROPERTY()
    TEnumAsByte<EAngularDriveMode::Type> AngularDriveMode = EAngularDriveMode::SLERP;
    UPROPERTY()
    bool bAccelerationMode = false;
    UPROPERTY()
    EAngularDriveLimitViolationResponse LimitViolationResponse = EAngularDriveLimitViolationResponse::None;
};

}
