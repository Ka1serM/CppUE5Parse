#pragma once

#include "ue5parse/runtime/CoreUObject/Templates/SubclassOf.h"
#include "ue5parse/runtime/Engine/Engine/EngineBaseTypes.h"

#include "EngineTypes.generated.h"

// The Engine module's shared types (UE's Engine/EngineTypes.h, Engine/NetSerialization.h and
// Engine/ReplicatedState.h): attachment rules, mobility, end play reasons, collision and rendering enums, and the
// replicated movement and attachment of actors.

namespace ue5parse::runtime {

class AActor;
class USceneComponent;
class UPrimitiveComponent;
class UPhysicalMaterial;
class UDamageType;

namespace EComponentMobility {
enum Type : int32 { Static, Stationary, Movable };
}

enum class EAttachmentRule : uint8 { KeepRelative, KeepWorld, SnapToTarget };
enum class EDetachmentRule : uint8 { KeepRelative, KeepWorld };

// How a component keeps its transform when it is attached (UE's FAttachmentTransformRules).
struct FAttachmentTransformRules {
    EAttachmentRule LocationRule = EAttachmentRule::KeepRelative;
    EAttachmentRule RotationRule = EAttachmentRule::KeepRelative;
    EAttachmentRule ScaleRule = EAttachmentRule::KeepRelative;
    bool bWeldSimulatedBodies = false;

    FAttachmentTransformRules() = default;
    FAttachmentTransformRules(EAttachmentRule InRule, bool bInWeldSimulatedBodies)
        : LocationRule(InRule), RotationRule(InRule), ScaleRule(InRule), bWeldSimulatedBodies(bInWeldSimulatedBodies) {}
    FAttachmentTransformRules(EAttachmentRule InLocationRule, EAttachmentRule InRotationRule, EAttachmentRule InScaleRule,
                              bool bInWeldSimulatedBodies)
        : LocationRule(InLocationRule), RotationRule(InRotationRule), ScaleRule(InScaleRule), bWeldSimulatedBodies(bInWeldSimulatedBodies) {}

    static const FAttachmentTransformRules KeepRelativeTransform;
    static const FAttachmentTransformRules KeepWorldTransform;
    static const FAttachmentTransformRules SnapToTargetNotIncludingScale;
    static const FAttachmentTransformRules SnapToTargetIncludingScale;
};

struct FDetachmentTransformRules {
    EDetachmentRule LocationRule = EDetachmentRule::KeepRelative;
    EDetachmentRule RotationRule = EDetachmentRule::KeepRelative;
    EDetachmentRule ScaleRule = EDetachmentRule::KeepRelative;
    bool bCallModify = true;

    FDetachmentTransformRules() = default;
    FDetachmentTransformRules(EDetachmentRule InRule, bool bInCallModify)
        : LocationRule(InRule), RotationRule(InRule), ScaleRule(InRule), bCallModify(bInCallModify) {}
    FDetachmentTransformRules(const FAttachmentTransformRules& AttachmentRules, bool bInCallModify)
        : LocationRule(AttachmentRules.LocationRule == EAttachmentRule::KeepRelative ? EDetachmentRule::KeepRelative : EDetachmentRule::KeepWorld),
          RotationRule(AttachmentRules.RotationRule == EAttachmentRule::KeepRelative ? EDetachmentRule::KeepRelative : EDetachmentRule::KeepWorld),
          ScaleRule(AttachmentRules.ScaleRule == EAttachmentRule::KeepRelative ? EDetachmentRule::KeepRelative : EDetachmentRule::KeepWorld),
          bCallModify(bInCallModify) {}

    static const FDetachmentTransformRules KeepRelativeTransform;
    static const FDetachmentTransformRules KeepWorldTransform;
};

namespace EEndPlayReason {
enum Type : int32 { Destroyed, LevelTransition, EndPlayInEditor, RemovedFromWorld, Quit };
}

enum class ETeleportType : uint8 { None, TeleportPhysics, ResetPhysics };

inline ETeleportType TeleportFlagToEnum(bool bTeleport) { return bTeleport ? ETeleportType::TeleportPhysics : ETeleportType::None; }
inline bool TeleportEnumToFlag(ETeleportType Teleport) { return Teleport == ETeleportType::TeleportPhysics; }

enum class ESpawnActorCollisionHandlingMethod : uint8 {
    Undefined,
    AlwaysSpawn,
    AdjustIfPossibleButAlwaysSpawn,
    AdjustIfPossibleButDontSpawnIfColliding,
    DontSpawnIfColliding,
};

enum class ESpawnActorScaleMethod : uint8 { OverrideRootScale, MultiplyWithRoot, SelectDefaultAtRuntime };

enum ECollisionChannel : int32 {
    ECC_WorldStatic,
    ECC_WorldDynamic,
    ECC_Pawn,
    ECC_Visibility,
    ECC_Camera,
    ECC_PhysicsBody,
    ECC_Vehicle,
    ECC_Destructible,
    ECC_EngineTraceChannel1,
    ECC_EngineTraceChannel2,
    ECC_EngineTraceChannel3,
    ECC_EngineTraceChannel4,
    ECC_EngineTraceChannel5,
    ECC_EngineTraceChannel6,
    ECC_GameTraceChannel1,
    ECC_GameTraceChannel2,
    ECC_GameTraceChannel3,
    ECC_GameTraceChannel4,
    ECC_GameTraceChannel5,
    ECC_GameTraceChannel6,
    ECC_GameTraceChannel7,
    ECC_GameTraceChannel8,
    ECC_GameTraceChannel9,
    ECC_GameTraceChannel10,
    ECC_GameTraceChannel11,
    ECC_GameTraceChannel12,
    ECC_GameTraceChannel13,
    ECC_GameTraceChannel14,
    ECC_GameTraceChannel15,
    ECC_GameTraceChannel16,
    ECC_GameTraceChannel17,
    ECC_GameTraceChannel18,
    ECC_OverlapAll_Deprecated,
    ECC_MAX,
};

enum ECollisionResponse : int32 { ECR_Ignore, ECR_Overlap, ECR_Block, ECR_MAX };

// The trace channels and object types of the collision queries of Blueprints (UE's EngineTypes.h).
enum ETraceTypeQuery {
    TraceTypeQuery1,
    TraceTypeQuery2,
    TraceTypeQuery3,
    TraceTypeQuery4,
    TraceTypeQuery5,
    TraceTypeQuery6,
    TraceTypeQuery7,
    TraceTypeQuery8,
    TraceTypeQuery9,
    TraceTypeQuery10,
    TraceTypeQuery11,
    TraceTypeQuery12,
    TraceTypeQuery13,
    TraceTypeQuery14,
    TraceTypeQuery15,
    TraceTypeQuery16,
    TraceTypeQuery17,
    TraceTypeQuery18,
    TraceTypeQuery19,
    TraceTypeQuery20,
    TraceTypeQuery21,
    TraceTypeQuery22,
    TraceTypeQuery23,
    TraceTypeQuery24,
    TraceTypeQuery25,
    TraceTypeQuery26,
    TraceTypeQuery27,
    TraceTypeQuery28,
    TraceTypeQuery29,
    TraceTypeQuery30,
    TraceTypeQuery31,
    TraceTypeQuery32,
    TraceTypeQuery_MAX,
};

enum EObjectTypeQuery {
    ObjectTypeQuery1,
    ObjectTypeQuery2,
    ObjectTypeQuery3,
    ObjectTypeQuery4,
    ObjectTypeQuery5,
    ObjectTypeQuery6,
    ObjectTypeQuery7,
    ObjectTypeQuery8,
    ObjectTypeQuery9,
    ObjectTypeQuery10,
    ObjectTypeQuery11,
    ObjectTypeQuery12,
    ObjectTypeQuery13,
    ObjectTypeQuery14,
    ObjectTypeQuery15,
    ObjectTypeQuery16,
    ObjectTypeQuery17,
    ObjectTypeQuery18,
    ObjectTypeQuery19,
    ObjectTypeQuery20,
    ObjectTypeQuery21,
    ObjectTypeQuery22,
    ObjectTypeQuery23,
    ObjectTypeQuery24,
    ObjectTypeQuery25,
    ObjectTypeQuery26,
    ObjectTypeQuery27,
    ObjectTypeQuery28,
    ObjectTypeQuery29,
    ObjectTypeQuery30,
    ObjectTypeQuery31,
    ObjectTypeQuery32,
    ObjectTypeQuery_MAX,
};


namespace ECollisionEnabled {
enum Type : int32 { NoCollision, QueryOnly, PhysicsOnly, QueryAndPhysics, ProbeOnly, QueryAndProbe };
}

enum EMovementMode : int32 { MOVE_None, MOVE_Walking, MOVE_NavWalking, MOVE_Falling, MOVE_Swimming, MOVE_Flying, MOVE_Custom, MOVE_MAX };

enum EBlendMode : int32 {
    BLEND_Opaque,
    BLEND_Masked,
    BLEND_Translucent,
    BLEND_Additive,
    BLEND_Modulate,
    BLEND_AlphaComposite,
    BLEND_AlphaHoldout,
    BLEND_TranslucentColoredTransmittance,
    BLEND_MAX,
    BLEND_TranslucentGreyTransmittance = BLEND_Translucent,
    BLEND_ColoredTransmittanceOnly = BLEND_Modulate,
};

enum EMaterialShadingModel : int32 {
    MSM_Unlit,
    MSM_DefaultLit,
    MSM_Subsurface,
    MSM_PreintegratedSkin,
    MSM_ClearCoat,
    MSM_SubsurfaceProfile,
    MSM_TwoSidedFoliage,
    MSM_Hair,
    MSM_Cloth,
    MSM_Eye,
    MSM_SingleLayerWater,
    MSM_ThinTranslucent,
    MSM_Strata,
    MSM_NUM,
    MSM_FromMaterialExpression,
    MSM_MAX,
};

// FLightingChannels: the three lighting channels a light or a primitive is in.
USTRUCT()
struct FLightingChannels {
    GENERATED_BODY()
    UPROPERTY()
    bool bChannel0 = true;
    UPROPERTY()
    bool bChannel1 = false;
    UPROPERTY()
    bool bChannel2 = false;
};

// The replicated vectors (Engine/NetSerialization.h): FVector values that the net serializer quantizes.
USTRUCT()
struct FVector_NetQuantize : public FVector {
    GENERATED_BODY()
    FVector_NetQuantize() = default;
    FVector_NetQuantize(const FVector& InVector) : FVector(InVector) {}
};

USTRUCT()
struct FVector_NetQuantize10 : public FVector {
    GENERATED_BODY()
    FVector_NetQuantize10() = default;
    FVector_NetQuantize10(const FVector& InVector) : FVector(InVector) {}
};

USTRUCT()
struct FVector_NetQuantize100 : public FVector {
    GENERATED_BODY()
    FVector_NetQuantize100() = default;
    FVector_NetQuantize100(const FVector& InVector) : FVector(InVector) {}
};

USTRUCT()
struct FVector_NetQuantizeNormal : public FVector {
    GENERATED_BODY()
    FVector_NetQuantizeNormal() = default;
    FVector_NetQuantizeNormal(const FVector& InVector) : FVector(InVector) {}
};

enum class EVectorQuantization : uint8 { RoundWholeNumber, RoundOneDecimal, RoundTwoDecimals };
enum class ERotatorQuantization : uint8 { ByteComponents, ShortComponents };

// FRepMovement: an actor's replicated movement (Engine/ReplicatedState.h).
USTRUCT()
struct FRepMovement {
    GENERATED_BODY()
    UPROPERTY()
    FVector LinearVelocity;
    UPROPERTY()
    FVector AngularVelocity;
    UPROPERTY()
    FVector Location;
    UPROPERTY()
    FRotator Rotation;
    UPROPERTY()
    bool bSimulatedPhysicSleep = false;
    UPROPERTY()
    bool bRepPhysics = false;
    UPROPERTY()
    int32 ServerFrame = 0;
    UPROPERTY()
    int32 ServerPhysicsHandle = INDEX_NONE;
    UPROPERTY()
    EVectorQuantization LocationQuantizationLevel = EVectorQuantization::RoundWholeNumber;
    UPROPERTY()
    EVectorQuantization VelocityQuantizationLevel = EVectorQuantization::RoundWholeNumber;
    UPROPERTY()
    ERotatorQuantization RotationQuantizationLevel = ERotatorQuantization::ByteComponents;
};

// FRepAttachment: an actor's replicated attachment.
USTRUCT()
struct FRepAttachment {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<AActor> AttachParent = nullptr;
    UPROPERTY()
    FVector_NetQuantize100 LocationOffset;
    UPROPERTY()
    FVector_NetQuantize100 RelativeScale3D;
    UPROPERTY()
    FRotator RotationOffset;
    UPROPERTY()
    FName AttachSocket;
    UPROPERTY()
    TObjectPtr<USceneComponent> AttachComponent = nullptr;
};

// FActorInstanceHandle: an actor, or an instance a primitive stands for (Engine/ActorInstanceHandle.h). The runtime
// keeps the actor only.
USTRUCT()
struct FActorInstanceHandle {
    GENERATED_BODY()
    UPROPERTY()
    TWeakObjectPtr<AActor> Actor;
    UPrimitiveComponent* ReferenceObject = nullptr;
    int32 InstanceIndex = INDEX_NONE;
    uint32 InstanceUID = 0;

    bool operator==(const FActorInstanceHandle& Other) const = default;
};

// FHitResult: the result of a trace or a sweep (Engine/HitResult.h). The runtime has no collision: Blueprints make and
// break these.
USTRUCT()
struct FHitResult {
    GENERATED_BODY()
    UPROPERTY()
    int32 FaceIndex = INDEX_NONE;
    UPROPERTY()
    float Time = 1.0f;
    UPROPERTY()
    float Distance = 0;
    UPROPERTY()
    FVector_NetQuantize Location;
    UPROPERTY()
    FVector_NetQuantize ImpactPoint;
    UPROPERTY()
    FVector_NetQuantizeNormal Normal;
    UPROPERTY()
    FVector_NetQuantizeNormal ImpactNormal;
    UPROPERTY()
    FVector_NetQuantize TraceStart;
    UPROPERTY()
    FVector_NetQuantize TraceEnd;
    UPROPERTY()
    float PenetrationDepth = 0;
    UPROPERTY()
    int32 MyItem = INDEX_NONE;
    UPROPERTY()
    int32 Item = INDEX_NONE;
    UPROPERTY()
    uint8 ElementIndex = 0;
    UPROPERTY()
    bool bBlockingHit = false;
    UPROPERTY()
    bool bStartPenetrating = false;
    UPROPERTY()
    TWeakObjectPtr<UPhysicalMaterial> PhysMaterial;
    UPROPERTY()
    FActorInstanceHandle HitObjectHandle;
    UPROPERTY()
    TWeakObjectPtr<UPrimitiveComponent> Component;
    UPROPERTY()
    FName BoneName;
    UPROPERTY()
    FName MyBoneName;

    AActor* GetActor() const { return HitObjectHandle.Actor.Get(); }
    UPrimitiveComponent* GetComponent() const { return Component.Get(); }
};

// FRadialDamageParams: the damage and falloff of a radial damage (Engine/EngineTypes.h).
USTRUCT()
struct FRadialDamageParams {
    GENERATED_BODY()
    UPROPERTY()
    float BaseDamage = 0;
    UPROPERTY()
    float MinimumDamage = 0;
    UPROPERTY()
    float InnerRadius = 0;
    UPROPERTY()
    float OuterRadius = 0;
    UPROPERTY()
    float DamageFalloff = 0;
};

// FDamageEvent and FRadialDamageEvent: the damage events that AActor::TakeDamage takes (Engine/DamageEvents.h),
// without their virtual class IDs.
USTRUCT()
struct FDamageEvent {
    GENERATED_BODY()
    UPROPERTY()
    TSubclassOf<UDamageType> DamageTypeClass;
};

USTRUCT()
struct FRadialDamageEvent : public FDamageEvent {
    GENERATED_BODY()
    UPROPERTY()
    FRadialDamageParams Params;
    UPROPERTY()
    FVector Origin;
    UPROPERTY()
    TArray<FHitResult> ComponentHits;
};

}
