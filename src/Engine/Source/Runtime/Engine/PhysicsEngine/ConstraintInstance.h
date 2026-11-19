#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/ConstraintDrives.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/ConstraintTypes.h"

#include "ConstraintInstance.generated.h"

namespace ue5parse::runtime {

class USceneComponent;

USTRUCT()
struct FConstraintProfileProperties {
    GENERATED_BODY()
    UPROPERTY()
    float ProjectionLinearTolerance = 0.0f;
    UPROPERTY()
    float ProjectionAngularTolerance = 0.0f;
    UPROPERTY()
    float ProjectionLinearAlpha = 0.0f;
    UPROPERTY()
    float ProjectionAngularAlpha = 0.0f;
    UPROPERTY()
    float ShockPropagationAlpha = 0.0f;
    UPROPERTY()
    float LinearBreakThreshold = 0.0f;
    UPROPERTY()
    float LinearPlasticityThreshold = 0.0f;
    UPROPERTY()
    float AngularBreakThreshold = 0.0f;
    UPROPERTY()
    float AngularPlasticityThreshold = 0.0f;
    UPROPERTY()
    float ContactTransferScale = 0.0f;
    UPROPERTY()
    FLinearConstraint LinearLimit;
    UPROPERTY()
    FConeConstraint ConeLimit;
    UPROPERTY()
    FTwistConstraint TwistLimit;
    UPROPERTY()
    FLinearDriveConstraint LinearDrive;
    UPROPERTY()
    FAngularDriveConstraint AngularDrive;
    UPROPERTY()
    bool bDisableCollision = false;
    UPROPERTY()
    bool bParentDominates = false;
    UPROPERTY()
    bool bEnableShockPropagation = false;
    UPROPERTY()
    bool bEnableProjection = false;
    UPROPERTY()
    bool bEnableSoftProjection = false;
    UPROPERTY()
    bool bEnableMassConditioning = false;
    UPROPERTY()
    bool bUseLinearJointSolver = false;
    UPROPERTY()
    bool bAngularBreakable = false;
    UPROPERTY()
    bool bAngularPlasticity = false;
    UPROPERTY()
    bool bLinearBreakable = false;
    UPROPERTY()
    bool bLinearPlasticity = false;
    UPROPERTY()
    TEnumAsByte<EConstraintPlasticityType> LinearPlasticityType;
};

USTRUCT()
struct FConstraintInstanceBase {
    GENERATED_BODY()
};

USTRUCT()
struct FConstraintInstance : public FConstraintInstanceBase {
    GENERATED_BODY()
    UPROPERTY()
    FName JointName;
    UPROPERTY()
    FName ConstraintBone1;
    UPROPERTY()
    FName ConstraintBone2;
    UPROPERTY()
    FVector Pos1 = FVector::ZeroVector;
    UPROPERTY()
    FVector PriAxis1 = FVector::ZeroVector;
    UPROPERTY()
    FVector SecAxis1 = FVector::ZeroVector;
    UPROPERTY()
    FVector Pos2 = FVector::ZeroVector;
    UPROPERTY()
    FVector PriAxis2 = FVector::ZeroVector;
    UPROPERTY()
    FVector SecAxis2 = FVector::ZeroVector;
    UPROPERTY()
    FRotator AngularRotationOffset = FRotator::ZeroRotator;
    UPROPERTY()
    bool bScaleLinearLimits = false;
    UPROPERTY()
    FConstraintProfileProperties ProfileInstance;
    //! UE 4.3 to 4.12 store the limits and drives flat on the instance.
    UPROPERTY()
    TObjectPtr<USceneComponent> OwnerComponent = nullptr;
    UPROPERTY()
    bool bDisableCollision = false;
    UPROPERTY()
    bool bEnableProjection = false;
    UPROPERTY()
    float ProjectionLinearTolerance = 0.0f;
    UPROPERTY()
    float ProjectionAngularTolerance = 0.0f;
    UPROPERTY()
    TEnumAsByte<ELinearConstraintMotion> LinearXMotion;
    UPROPERTY()
    TEnumAsByte<ELinearConstraintMotion> LinearYMotion;
    UPROPERTY()
    TEnumAsByte<ELinearConstraintMotion> LinearZMotion;
    UPROPERTY()
    float LinearLimitSize = 0.0f;
    UPROPERTY()
    bool bLinearLimitSoft = false;
    UPROPERTY()
    float LinearLimitStiffness = 0.0f;
    UPROPERTY()
    float LinearLimitDamping = 0.0f;
    UPROPERTY()
    bool bLinearBreakable = false;
    UPROPERTY()
    float LinearBreakThreshold = 0.0f;
    UPROPERTY()
    TEnumAsByte<EAngularConstraintMotion> AngularSwing1Motion;
    UPROPERTY()
    TEnumAsByte<EAngularConstraintMotion> AngularSwing2Motion;
    UPROPERTY()
    TEnumAsByte<EAngularConstraintMotion> AngularTwistMotion;
    UPROPERTY()
    bool bSwingLimitSoft = false;
    UPROPERTY()
    bool bTwistLimitSoft = false;
    UPROPERTY()
    float Swing1LimitAngle = 0.0f;
    UPROPERTY()
    float Swing2LimitAngle = 0.0f;
    UPROPERTY()
    float TwistLimitAngle = 0.0f;
    UPROPERTY()
    float SwingLimitStiffness = 0.0f;
    UPROPERTY()
    float SwingLimitDamping = 0.0f;
    UPROPERTY()
    float TwistLimitStiffness = 0.0f;
    UPROPERTY()
    float TwistLimitDamping = 0.0f;
    UPROPERTY()
    bool bAngularBreakable = false;
    UPROPERTY()
    float AngularBreakThreshold = 0.0f;
    UPROPERTY()
    bool bLinearXPositionDrive = false;
    UPROPERTY()
    bool bLinearYPositionDrive = false;
    UPROPERTY()
    bool bLinearZPositionDrive = false;
    UPROPERTY()
    bool bLinearPositionDrive = false;
    UPROPERTY()
    bool bLinearVelocityDrive = false;
    UPROPERTY()
    bool bLinearXVelocityDrive = false;
    UPROPERTY()
    bool bLinearYVelocityDrive = false;
    UPROPERTY()
    bool bLinearZVelocityDrive = false;
    UPROPERTY()
    FVector LinearPositionTarget = FVector::ZeroVector;
    UPROPERTY()
    FVector LinearVelocityTarget = FVector::ZeroVector;
    UPROPERTY()
    float LinearDriveSpring = 0.0f;
    UPROPERTY()
    float LinearDriveDamping = 0.0f;
    UPROPERTY()
    float LinearDriveForceLimit = 0.0f;
    UPROPERTY()
    bool bAngularOrientationDrive = false;
    UPROPERTY()
    bool bAngularVelocityDrive = false;
    UPROPERTY()
    bool bEnableSwingDrive = false;
    UPROPERTY()
    bool bEnableTwistDrive = false;
    UPROPERTY()
    TEnumAsByte<EAngularDriveMode::Type> AngularDriveMode;
    UPROPERTY()
    FRotator AngularOrientationTarget = FRotator::ZeroRotator;
    UPROPERTY()
    FVector AngularVelocityTarget = FVector::ZeroVector;
    UPROPERTY()
    float AngularDriveSpring = 0.0f;
    UPROPERTY()
    float AngularDriveDamping = 0.0f;
    UPROPERTY()
    float AngularDriveForceLimit = 0.0f;
};

}
