#pragma once

#include "Engine/Source/Runtime/Engine/AI/Navigation/NavigationAvoidanceTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineBaseTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/GameFramework/PawnMovementComponent.h"

#include "CharacterMovementComponent.generated.h"

namespace ue5parse::runtime {

class ACharacter;

USTRUCT()
struct FFindFloorResult {
    GENERATED_BODY()
    UPROPERTY()
    uint8 bBlockingHit : 1 = 0;
    UPROPERTY()
    uint8 bWalkableFloor : 1 = 0;
    UPROPERTY()
    uint8 bLineTrace : 1 = 0;
    UPROPERTY()
    float FloorDist = 0;
    UPROPERTY()
    float LineDist = 0;
    UPROPERTY()
    FHitResult HitResult;
};

USTRUCT()
struct FCharacterMovementComponentPreClothTickFunction : public FTickFunction {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FCharacterMovementComponentPreClothTickFunction> : TStructOpsTypeTraitsBase2<FCharacterMovementComponentPreClothTickFunction> {
    enum { WithCopy = false };
};

USTRUCT()
struct FCharacterMovementComponentPostPhysicsTickFunction : public FTickFunction {
    GENERATED_BODY()
};

template <>
struct TStructOpsTypeTraits<FCharacterMovementComponentPostPhysicsTickFunction>
    : TStructOpsTypeTraitsBase2<FCharacterMovementComponentPostPhysicsTickFunction> {
    enum { WithCopy = false };
};

UCLASS()
class UCharacterMovementComponent : public UPawnMovementComponent {
    GENERATED_BODY()
public:
    explicit UCharacterMovementComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    FCharacterMovementComponentPreClothTickFunction PreClothComponentTick;
    UPROPERTY(Transient)
    TObjectPtr<ACharacter> CharacterOwner = nullptr;
    UPROPERTY()
    float MaxStepHeight = 45.0f;
    UPROPERTY()
    float JumpZVelocity = 420.0f;
    UPROPERTY()
    float JumpOffJumpZFactor = 0.5f;
    UPROPERTY()
    float WalkableFloorAngle = 44.765f;
    UPROPERTY()
    float WalkableFloorZ = 0.71f;
    UPROPERTY()
    float PerchRadiusThreshold = 0.0f;
    UPROPERTY()
    float PerchAdditionalHeight = 40.f;
    UPROPERTY()
    TEnumAsByte<EMovementMode> MovementMode{};
    UPROPERTY()
    uint8 CustomMovementMode = 0;
    UPROPERTY()
    float GravityScale = 1.f;
    UPROPERTY()
    float GroundFriction = 8.0f;
    UPROPERTY()
    float MaxWalkSpeed = 600.0f;
    UPROPERTY()
    float MaxWalkSpeedCrouched = 300.0f;
    UPROPERTY()
    float MaxCustomMovementSpeed = 600.0f;
    UPROPERTY()
    float CrouchedHalfHeight = 40.0f;
    UPROPERTY()
    float MaxSwimSpeed = 300.0f;
    UPROPERTY()
    float MaxFlySpeed = 600.0f;
    UPROPERTY()
    float Buoyancy = 1.0f;
    UPROPERTY()
    FRotator RotationRate = FRotator(0.f, 360.0f, 0.0f);
    UPROPERTY()
    uint8 bUseControllerDesiredRotation : 1 = false;
    UPROPERTY()
    uint8 bOrientRotationToMovement : 1 = 0;
    UPROPERTY()
    uint8 bMovementInProgress : 1 = 0;
    UPROPERTY()
    uint8 bEnableScopedMovementUpdates : 1 = true;
    UPROPERTY()
    uint8 bForceMaxAccel : 1 = 0;
    UPROPERTY()
    uint8 bRunPhysicsWithNoController : 1 = 0;
    UPROPERTY()
    uint8 bForceNextFloorCheck : 1 = true;
    UPROPERTY()
    uint8 bShrinkProxyCapsule : 1 = true;
    UPROPERTY()
    uint8 bCanWalkOffLedges : 1 = true;
    UPROPERTY()
    uint8 bCanWalkOffLedgesWhenCrouching : 1 = false;
    UPROPERTY()
    uint8 bDeferUpdateMoveComponent : 1 = 0;
    UPROPERTY()
    TObjectPtr<USceneComponent> DeferredUpdatedMoveComponent = nullptr;
    UPROPERTY()
    float MaxOutOfWaterStepHeight = 40.0f;
    UPROPERTY()
    float OutofWaterZ = 420.0f;
    UPROPERTY()
    float AirControl = 0.05f;
    UPROPERTY()
    float FallingLateralFriction = 0.f;
    UPROPERTY()
    float Mass = 100.0f;
    UPROPERTY()
    uint8 bEnablePhysicsInteraction : 1 = true;
    UPROPERTY()
    uint8 bTouchForceScaledToMass : 1 = true;
    UPROPERTY()
    uint8 bPushForceScaledToMass : 1 = false;
    UPROPERTY()
    uint8 bScalePushForceToVelocity : 1 = true;
    UPROPERTY()
    float InitialPushForceFactor = 500.0f;
    UPROPERTY()
    float PushForceFactor = 750000.0f;
    UPROPERTY()
    float PushForcePointZOffsetFactor = -0.75f;
    UPROPERTY()
    float TouchForceFactor = 1.0f;
    UPROPERTY()
    float MinTouchForce = -1.0f;
    UPROPERTY()
    float MaxTouchForce = 250.0f;
    UPROPERTY()
    float RepulsionForce = 2.5f;
    UPROPERTY()
    uint8 bForceBraking_DEPRECATED : 1 = false;
    UPROPERTY()
    float CrouchedSpeedMultiplier_DEPRECATED = 0.5f;
    UPROPERTY()
    FVector Acceleration;
    UPROPERTY()
    FVector LastUpdateLocation;
    UPROPERTY()
    FVector PendingImpulseToApply = FVector::ZeroVector;
    UPROPERTY()
    FVector PendingForceToApply;
    UPROPERTY()
    float AnalogInputModifier = 0;
    UPROPERTY()
    float MaxSimulationTimeStep = 0.05f;
    UPROPERTY()
    int32 MaxSimulationIterations = 8;
    UPROPERTY()
    float MaxAcceleration = 2048.0f;
    UPROPERTY()
    float BrakingDecelerationWalking = 2048.0f;
    UPROPERTY()
    float BrakingDecelerationFalling = 0.f;
    UPROPERTY()
    float BrakingDecelerationSwimming = 0.f;
    UPROPERTY()
    float BrakingDecelerationFlying = 0.f;
    UPROPERTY()
    float LedgeCheckThreshold = 4.0f;
    UPROPERTY()
    float JumpOutOfWaterPitch = 11.25f;
    UPROPERTY()
    FFindFloorResult CurrentFloor;
    UPROPERTY()
    TEnumAsByte<EMovementMode> DefaultLandMovementMode = MOVE_Walking;
    UPROPERTY()
    TEnumAsByte<EMovementMode> DefaultWaterMovementMode = MOVE_Swimming;
    UPROPERTY()
    uint8 bMaintainHorizontalGroundVelocity : 1 = true;
    UPROPERTY()
    uint8 bImpartBaseVelocityX : 1 = true;
    UPROPERTY()
    uint8 bImpartBaseVelocityY : 1 = true;
    UPROPERTY()
    uint8 bImpartBaseVelocityZ : 1 = true;
    UPROPERTY()
    uint8 bImpartBaseAngularVelocity : 1 = true;
    UPROPERTY()
    uint8 bNotifyApex : 1 = 0;
    UPROPERTY()
    uint8 bCheatFlying : 1 = 0;
    UPROPERTY()
    uint8 bWantsToCrouch : 1 = 0;
    UPROPERTY()
    uint8 bCrouchMovesCharacterDown : 1 = 0;
    UPROPERTY()
    uint8 bIgnoreBaseRotation : 1 = 0;
    UPROPERTY()
    uint8 bFastAttachedMove : 1 = 0;
    UPROPERTY()
    uint8 bAlwaysCheckFloor : 1 = true;
    UPROPERTY()
    uint8 bUseFlatBaseForFloorChecks : 1 = 0;
    UPROPERTY()
    uint8 bPerformingJumpOff : 1 = 0;
    UPROPERTY()
    uint8 bUseRVOAvoidance : 1 = false;
    UPROPERTY()
    uint8 bRequestedMoveUseAcceleration : 1 = true;
    UPROPERTY()
    int32 AvoidanceUID = 0;
    UPROPERTY()
    FNavAvoidanceMask AvoidanceGroup;
    UPROPERTY()
    FNavAvoidanceMask GroupsToAvoid;
    UPROPERTY()
    FNavAvoidanceMask GroupsToIgnore;
    UPROPERTY()
    float AvoidanceWeight = 0;
    UPROPERTY()
    FVector PendingLaunchVelocity = FVector::ZeroVector;
    UPROPERTY()
    float MinTimeBetweenTimeStampResets = 4.f * 60.f;
    UPROPERTY()
    float AirControlBoostMultiplier = 2.f;
    UPROPERTY()
    float AirControlBoostVelocityThreshold = 25.f;
    UPROPERTY()
    float StandingDownwardForceScale = 1.0f;
    UPROPERTY()
    uint8 bCrouchMaintainsBaseLocation : 1 = 0;
    UPROPERTY()
    float UpperImpactNormalScale_DEPRECATED = 0.5f;
    UPROPERTY()
    float AvoidanceConsiderationRadius = 500.0f;
    UPROPERTY()
    float BrakingFrictionFactor = 2.0f;
    UPROPERTY()
    float BrakingFriction = 0;
    UPROPERTY()
    uint8 bUseSeparateBrakingFriction : 1 = false;
    UPROPERTY()
    FVector LastUpdateVelocity = FVector::ZeroVector;
    UPROPERTY()
    float NetworkSimulatedSmoothLocationTime = 0.100f;
    UPROPERTY()
    float NetworkSimulatedSmoothRotationTime = 0.050f;
    UPROPERTY()
    float NetworkMaxSmoothUpdateDistance = 256.f;
    UPROPERTY()
    float NetworkNoSmoothUpdateDistance = 384.f;
    UPROPERTY()
    ENetworkSmoothingMode NetworkSmoothingMode = ENetworkSmoothingMode::Exponential;
    UPROPERTY()
    uint8 bWantsToLeaveNavWalking : 1 = false;
    UPROPERTY()
    uint8 bProjectNavMeshWalking : 1 = false;
    UPROPERTY()
    uint8 bProjectNavMeshOnBothWorldChannels : 1 = 0;
    UPROPERTY()
    float NavMeshProjectionInterval = 0.1f;
    UPROPERTY()
    float NavMeshProjectionInterpSpeed = 12.f;
    UPROPERTY()
    float NavMeshProjectionHeightScaleUp = 0.67f;
    UPROPERTY()
    float NavMeshProjectionHeightScaleDown = 1.0f;
    UPROPERTY()
    FCharacterMovementComponentPostPhysicsTickFunction PostPhysicsTickFunction;
    UPROPERTY()
    uint8 bPushForceUsingZOffset : 1 = false;
    UPROPERTY()
    FQuat LastUpdateRotation = FQuat::Identity;
    UPROPERTY()
    float ListenServerNetworkSimulatedSmoothLocationTime = 0.040f;
    UPROPERTY()
    float ListenServerNetworkSimulatedSmoothRotationTime = 0.033f;
    UPROPERTY()
    float MaxDepenetrationWithGeometry = 500.f;
    UPROPERTY()
    float MaxDepenetrationWithGeometryAsProxy = 100.f;
    UPROPERTY()
    float MaxDepenetrationWithPawn = 100.f;
    UPROPERTY()
    float MaxDepenetrationWithPawnAsProxy = 2.f;
    UPROPERTY()
    float NavWalkingFloorDistTolerance = 10.0f;
    UPROPERTY()
    uint8 bApplyGravityWhileJumping : 1 = true;
    UPROPERTY()
    uint8 bSweepWhileNavWalking : 1 = true;
    UPROPERTY()
    float NetProxyShrinkRadius = 0.01f;
    UPROPERTY()
    float NetProxyShrinkHalfHeight = 0.01f;
    UPROPERTY()
    uint8 bAllowPhysicsRotationDuringAnimRootMotion : 1 = false;
    UPROPERTY()
    float MinAnalogWalkSpeed = 0;
    UPROPERTY()
    uint8 bNetworkSkipProxyPredictionOnNetUpdate : 1 = 0;
    UPROPERTY()
    float NetworkMinTimeBetweenClientAckGoodMoves = 0.10f;
    UPROPERTY()
    float NetworkMinTimeBetweenClientAdjustments = 0.10f;
    UPROPERTY()
    float NetworkMinTimeBetweenClientAdjustmentsLargeCorrection = 0.05f;
    UPROPERTY()
    float NetworkLargeClientCorrectionDistance = 15.0f;
    UPROPERTY()
    uint8 bNetworkAlwaysReplicateTransformUpdateTimestamp : 1 = 0;
    UPROPERTY()
    float BrakingSubStepTime = 1.0f / 33.0f;
    UPROPERTY()
    uint8 bEnableServerDualMoveScopedMovementUpdates : 1 = false;
    UPROPERTY()
    int32 MaxJumpApexAttemptsPerSimulation = 2;
    UPROPERTY()
    float FormerBaseVelocityDecayHalfLife = 0.f;
    UPROPERTY()
    FVector GravityDirection = FVector(0.0, 0.0, -1.0);
    UPROPERTY()
    FQuat WorldToGravityTransform = FQuat::Identity;
    UPROPERTY()
    FQuat GravityToWorldTransform = FQuat::Identity;
    UPROPERTY()
    bool bBasedMovementIgnorePhysicsBase = false;
    UPROPERTY()
    bool bStayBasedInAir = false;
    UPROPERTY()
    float StayBasedInAirHeight = 1000.0f;
    UPROPERTY()
    uint8 bDontFallBelowJumpZVelocityDuringJump : 1 = true;
    UPROPERTY()
    bool bBaseOnAttachmentRoot = false;

    UFUNCTION(BlueprintCallable)
    FVector GetCurrentAcceleration() const { return Acceleration; }
    UFUNCTION(BlueprintCallable)
    bool IsFalling() const { return MovementMode == MOVE_Falling && UpdatedComponent != nullptr; }
    UFUNCTION(BlueprintCallable)
    bool IsMovingOnGround() const { return (MovementMode == MOVE_Walking || MovementMode == MOVE_NavWalking) && UpdatedComponent != nullptr; }
    UFUNCTION(BlueprintCallable)
    bool IsFlying() const { return MovementMode == MOVE_Flying && UpdatedComponent != nullptr; }
    virtual bool CanCrouchInCurrentState() const { return IsFalling() || IsMovingOnGround(); }
    void ApplyNetworkMovementMode(uint8 ReceivedMode);
    void SetUpdatedComponent(USceneComponent* NewUpdatedComponent) override;
};

}
