#pragma once

#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/PhysicsCore/BodyInstanceCore.h"
#include "Engine/Source/Runtime/PhysicsCore/Chaos/ChaosEngineInterface.h"

#include "BodyInstance.generated.h"

namespace ue5parse::runtime {

class UPhysicalMaterial;

UENUM()
enum class EDynamicActorScene : uint8 { Default, UseSyncScene, UseAsyncScene };

USTRUCT()
struct FCollisionResponse {
    GENERATED_BODY()
    UPROPERTY()
    FCollisionResponseContainer ResponseToChannels;
    UPROPERTY()
    TArray<FResponseChannel> ResponseArray;
};

namespace EDOFMode {

UENUM()
enum Type : int32 { Default, SixDOF, YZPlane, XZPlane, XYPlane, CustomPlane, None };

}

USTRUCT()
struct FBodyInstance : public FBodyInstanceCore {
    GENERATED_BODY()
    UPROPERTY()
    TEnumAsByte<ECollisionEnabled::Type> CollisionEnabled = ECollisionEnabled::QueryAndPhysics;
    UPROPERTY()
    FName CollisionProfileName;
    UPROPERTY()
    TEnumAsByte<ECollisionChannel> ObjectType = ECC_WorldStatic;
    UPROPERTY()
    FCollisionResponse CollisionResponses;
    UPROPERTY()
    TEnumAsByte<EDOFMode::Type> DOFMode = EDOFMode::Default;
    UPROPERTY()
    bool bUseCCD = false;
    UPROPERTY()
    bool bUseMACD = false;
    UPROPERTY()
    bool bIgnoreAnalyticCollisions = false;
    UPROPERTY()
    bool bNotifyRigidBodyCollision = false;
    UPROPERTY()
    bool bAllowPartialIslandSleep = false;
    UPROPERTY()
    bool bSmoothEdgeCollisions = false;
    UPROPERTY()
    bool bLockTranslation = true;
    UPROPERTY()
    bool bLockRotation = true;
    UPROPERTY()
    bool bLockXTranslation = false;
    UPROPERTY()
    bool bLockYTranslation = false;
    UPROPERTY()
    bool bLockZTranslation = false;
    UPROPERTY()
    bool bLockXRotation = false;
    UPROPERTY()
    bool bLockYRotation = false;
    UPROPERTY()
    bool bLockZRotation = false;
    UPROPERTY()
    bool bOverrideMaxAngularVelocity = false;
    UPROPERTY()
    bool bOverrideMaxDepenetrationVelocity = false;
    UPROPERTY()
    bool bOverrideWalkableSlopeOnInstance = false;
    UPROPERTY()
    bool bInterpolateWhenSubStepping = true;
    UPROPERTY()
    bool bOverrideIterationCounts = false;
    UPROPERTY()
    bool bInertiaConditioning = true;
    UPROPERTY()
    bool bOneWayInteraction = false;
    UPROPERTY()
    bool bOverrideSolverAsyncDeltaTime = false;
    UPROPERTY()
    bool bUseAsyncScene = false;
    UPROPERTY()
    float SolverAsyncDeltaTime = 0.0f;
    UPROPERTY()
    float MaxDepenetrationVelocity = 0.0f;
    UPROPERTY()
    float MassInKgOverride = 100.0f;
    //! UE 4.8 to 4.11 save the mass override as MassInKg.
    UPROPERTY()
    float MassInKg = 100.0f;
    UPROPERTY()
    float LinearDamping = 0.01f;
    UPROPERTY()
    float AngularDamping = 0.0f;
    UPROPERTY()
    FVector CustomDOFPlaneNormal = FVector::ZeroVector;
    UPROPERTY()
    FVector COMNudge = FVector::ZeroVector;
    UPROPERTY()
    float MassScale = 1.0f;
    UPROPERTY()
    FVector InertiaTensorScale = FVector::OneVector;
    UPROPERTY()
    FWalkableSlopeOverride WalkableSlopeOverride;
    UPROPERTY()
    TObjectPtr<UPhysicalMaterial> PhysMaterialOverride = nullptr;
    UPROPERTY()
    float MaxAngularVelocity = 400.0f;
    UPROPERTY()
    float CustomSleepThresholdMultiplier = 1.0f;
    UPROPERTY()
    float StabilizationThresholdMultiplier = 1.0f;
    UPROPERTY()
    float PhysicsBlendWeight = 0.0f;
    UPROPERTY()
    ESleepFamily SleepFamily = ESleepFamily::Normal;
    UPROPERTY()
    uint8 PositionSolverIterationCount = 8;
    UPROPERTY()
    uint8 VelocitySolverIterationCount = 1;
    UPROPERTY()
    uint8 ProjectionSolverIterationCount = 1;
    UPROPERTY()
    uint8 GravityGroupIndex = 0;
    UPROPERTY()
    uint64 RigidActorSyncId = 0;
    UPROPERTY()
    uint64 RigidActorAsyncId = 0;
    //! UE 4.3 to 4.11 save the body scale.
    UPROPERTY()
    FVector Scale3D = FVector::OneVector;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FBodyInstance> : TStructOpsTypeTraitsBase2<FBodyInstance> {
    enum { WithSerializer = true };
};

}
