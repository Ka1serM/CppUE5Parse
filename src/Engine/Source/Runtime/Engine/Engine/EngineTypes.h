#pragma once

#include "Engine/Source/Runtime/CoreUObject/Templates/SubclassOf.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ScriptDelegates.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineBaseTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/NaniteAssemblyData.h"

#include "EngineTypes.generated.h"

namespace ue5parse::runtime {

class AActor;
class USceneComponent;
class UPrimitiveComponent;
class UPhysicalMaterial;
class UDamageType;
class UTexture2D;

UENUM()
enum EFilterInterpolationType : int32 {
    BSIT_Average,
    BSIT_Linear,
    BSIT_Cubic,
    BSIT_EaseInOut,
    BSIT_ExponentialDecay,
    BSIT_SpringDamper,
};

UENUM()
enum ETrailWidthMode : int32 { ETrailWidthMode_FromCentre, ETrailWidthMode_FromFirst, ETrailWidthMode_FromSecond };

namespace EParticleCollisionMode {

UENUM()
enum Type : int32 { SceneDepth, DistanceField };

}

UENUM()
enum ESceneDepthPriorityGroup : int32 { SDPG_World, SDPG_Foreground, SDPG_MAX };

UENUM()
enum EIndirectLightingCacheQuality : int32 { ILCQ_Off, ILCQ_Point, ILCQ_Volume };

UENUM()
enum class ELightmapType : uint8 { Default, ForceSurface, ForceVolumetric };

namespace EComponentMobility {

UENUM()
enum Type : int32 { Static, Stationary, Movable };

}

UENUM()
enum class EAttachmentRule : uint8 { KeepRelative, KeepWorld, SnapToTarget };

namespace EAttachLocation {

UENUM()
enum Type : int32 { KeepRelativeOffset, KeepWorldPosition, SnapToTarget, SnapToTargetIncludingScale };

}
UENUM()
enum class EDetachmentRule : uint8 { KeepRelative, KeepWorld };

struct FAttachmentTransformRules {
    EAttachmentRule LocationRule = EAttachmentRule::KeepRelative;
    EAttachmentRule RotationRule = EAttachmentRule::KeepRelative;
    EAttachmentRule ScaleRule = EAttachmentRule::KeepRelative;
    bool bWeldSimulatedBodies = false;

    FAttachmentTransformRules() = default;
    FAttachmentTransformRules(EAttachmentRule InRule, bool bInWeldSimulatedBodies)
        : LocationRule(InRule), RotationRule(InRule), ScaleRule(InRule), bWeldSimulatedBodies(bInWeldSimulatedBodies) {}
    FAttachmentTransformRules(EAttachmentRule InLocationRule, EAttachmentRule InRotationRule, EAttachmentRule InScaleRule, bool bInWeldSimulatedBodies)
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

UENUM()
enum Type : int32 { Destroyed, LevelTransition, EndPlayInEditor, RemovedFromWorld, Quit };

}

enum class ETeleportType : uint8 { None, TeleportPhysics, ResetPhysics };

inline ETeleportType TeleportFlagToEnum(bool bTeleport) {
    return bTeleport ? ETeleportType::TeleportPhysics : ETeleportType::None;
}

UENUM()
enum ELightingBuildQuality : int32 { Quality_Preview, Quality_Medium, Quality_High, Quality_Production, Quality_MAX };

UENUM()
enum ENetDormancy : int32 { DORM_Never, DORM_Awake, DORM_DormantAll, DORM_DormantPartial, DORM_Initial, DORM_MAX };

namespace EDynamicGlobalIlluminationMethod {

UENUM()
enum Type : int32 { None, Lumen, ScreenSpace, Plugin };

}

namespace EReflectionMethod {

UENUM()
enum Type : int32 { None, Lumen, ScreenSpace };

}

namespace ECastRayTracedShadow {

UENUM()
enum Type : int32 { Disabled, UseProjectSetting, Enabled };

}

namespace EMegaLightsShadowMethod {

UENUM()
enum Type : int32 { Default, RayTracing, VirtualShadowMap };

}

UENUM()
enum ESceneCaptureSource : int32 {
    SCS_SceneColorHDR,
    SCS_SceneColorHDRNoAlpha,
    SCS_FinalColorLDR,
    SCS_SceneColorSceneDepth,
    SCS_SceneDepth,
    SCS_DeviceDepth,
    SCS_Normal,
    SCS_BaseColor,
    SCS_FinalColorHDR,
    SCS_FinalToneCurveHDR,
    SCS_MAX
};

UENUM()
enum ESceneCaptureCompositeMode : int32 { SCCM_Overwrite, SCCM_Additive, SCCM_Composite };

UENUM()
enum EOcclusionCombineMode : int32 { OCM_Minimum, OCM_Multiply, OCM_MAX };

UENUM()
enum EAspectRatioAxisConstraint : int32 { AspectRatio_MaintainYFOV, AspectRatio_MaintainXFOV, AspectRatio_MajorAxisFOV, AspectRatio_MAX };

namespace EAutoReceiveInput {

UENUM()
enum Type : int32 { Disabled, Player0, Player1, Player2, Player3, Player4, Player5, Player6, Player7 };

}

UENUM()
enum EInputConsumeOptions : int32 { ICO_ConsumeAll, ICO_ConsumeBoundKeys, ICO_ConsumeNone, ICO_MAX };

UENUM()
enum class EPhysicsReplicationMode : uint8 { Default = 0, PredictiveInterpolation = 1, Resimulation = 2, None = 255 };

UENUM()
enum class ESpawnActorCollisionHandlingMethod : uint8 {
    Undefined,
    AlwaysSpawn,
    AdjustIfPossibleButAlwaysSpawn,
    AdjustIfPossibleButDontSpawnIfColliding,
    DontSpawnIfColliding,
};

UENUM()
enum class ESpawnActorScaleMethod : uint8 { OverrideRootScale, MultiplyWithRoot, SelectDefaultAtRuntime };

UENUM()
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

UENUM()
enum ECollisionResponse : int32 { ECR_Ignore, ECR_Overlap, ECR_Block, ECR_MAX };

UENUM()
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

UENUM()
enum Type : int32 { NoCollision, QueryOnly, PhysicsOnly, QueryAndPhysics, ProbeOnly, QueryAndProbe };

}

UENUM()
enum class ENetworkSmoothingMode : uint8 { Disabled, Linear, Exponential, Replay };

UENUM()
enum EMovementMode : int32 { MOVE_None, MOVE_Walking, MOVE_NavWalking, MOVE_Falling, MOVE_Swimming, MOVE_Flying, MOVE_Custom, MOVE_MAX };

UENUM()
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

UENUM()
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

USTRUCT()
struct FMaterialShadingModelField {
    GENERATED_BODY()
    FMaterialShadingModelField() = default;
    explicit FMaterialShadingModelField(EMaterialShadingModel InShadingModel) : ShadingModelField(1 << InShadingModel) {}

    UPROPERTY()
    uint16 ShadingModelField = 0;
};

UENUM()
enum EMaterialFloatPrecisionMode : int32 {
    MFPM_Default,
    MFPM_Full_MaterialExpressionOnly,
    MFPM_Full,
    MFPM_Half,
    MFPM_Half_WithShaderParameters,
    MFPM_MAX,
};

UENUM()
enum ETranslucencyLightingMode : int32 {
    TLM_VolumetricNonDirectional,
    TLM_VolumetricDirectional,
    TLM_VolumetricPerVertexNonDirectional,
    TLM_VolumetricPerVertexDirectional,
    TLM_Surface,
    TLM_SurfacePerPixelLighting,
    TLM_MAX,
};

UENUM()
enum ERefractionMode : int32 { RM_IndexOfRefraction, RM_PixelNormalOffset, RM_2DOffset, RM_None, RM_IndexOfRefractionFromF0 };

UENUM()
enum ERefractionCoverageMode : int32 { RCM_CoverageIgnored, RCM_CoverageAccountedFor };

UENUM()
enum EPixelDepthOffsetMode : int32 { PDOM_Legacy, PDOM_AlongCameraVector };

UENUM()
enum EMaterialTessellationMode : int32 { MTM_NoTessellation, MTM_FlatTessellation, MTM_PNTriangles, MTM_MAX };

UENUM()
enum EStrataBlendMode : int32 {
    SBM_Opaque,
    SBM_Masked,
    SBM_TranslucentGreyTransmittance,
    SBM_TranslucentColoredTransmittance,
    SBM_ColoredTransmittanceOnly,
    SBM_AlphaHoldout,
    SBM_MAX,
};

UENUM()
enum ESamplerSourceMode : int32 { SSM_FromTextureAsset, SSM_Wrap_WorldGroupSettings, SSM_Clamp_WorldGroupSettings, SSM_TerrainWeightmapGroupSettings };

UENUM()
enum EMaterialSamplerType : int32 {
    SAMPLERTYPE_Color,
    SAMPLERTYPE_Grayscale,
    SAMPLERTYPE_Alpha,
    SAMPLERTYPE_Normal,
    SAMPLERTYPE_Masks,
    SAMPLERTYPE_DistanceFieldFont,
    SAMPLERTYPE_LinearColor,
    SAMPLERTYPE_LinearGrayscale,
    SAMPLERTYPE_Data,
    SAMPLERTYPE_External,
    SAMPLERTYPE_VirtualColor,
    SAMPLERTYPE_VirtualGrayscale,
    SAMPLERTYPE_VirtualAlpha,
    SAMPLERTYPE_VirtualNormal,
    SAMPLERTYPE_VirtualMasks,
    SAMPLERTYPE_VirtualLinearColor,
    SAMPLERTYPE_VirtualLinearGrayscale,
    SAMPLERTYPE_MAX,
};

UENUM()
enum EMaterialStencilCompare : int32 {
    MSC_Less,
    MSC_LessEqual,
    MSC_Greater,
    MSC_GreaterEqual,
    MSC_Equal,
    MSC_NotEqual,
    MSC_Never,
    MSC_Always,
    MSC_Count,
};

UENUM()
enum EMaterialShadingRate : int32 { MSR_1x1, MSR_2x1, MSR_1x2, MSR_2x2, MSR_4x2, MSR_2x4, MSR_4x4, MSR_Count };

USTRUCT()
struct FDisplacementScaling {
    GENERATED_BODY()
    UPROPERTY()
    float Magnitude = 4.0f;
    UPROPERTY()
    float Center = 0.5f;
};

USTRUCT()
struct FDisplacementFadeRange {
    GENERATED_BODY()
    UPROPERTY()
    float StartSizePixels = 4.0f;
    UPROPERTY()
    float EndSizePixels = 1.0f;
};

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

USTRUCT()
struct FViewLightingChannels {
    GENERATED_BODY()
    UPROPERTY()
    uint8 bViewChannel0 : 1 = true;
    UPROPERTY()
    uint8 bViewChannel1 : 1 = false;
    UPROPERTY()
    uint8 bViewChannel2 : 1 = false;
    UPROPERTY()
    uint8 bViewChannel3 : 1 = false;
    UPROPERTY()
    uint8 bViewChannel4 : 1 = false;
};

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

UENUM()
enum class EVectorQuantization : uint8 { RoundWholeNumber, RoundOneDecimal, RoundTwoDecimals };
UENUM()
enum class ERotatorQuantization : uint8 { ByteComponents, ShortComponents };

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

USTRUCT()
struct FActorInstanceHandle {
    GENERATED_BODY()
    UPROPERTY()
    TWeakObjectPtr<AActor> Actor;
    UPrimitiveComponent* ReferenceObject = nullptr;
    int32 InstanceIndex = INDEX_NONE;
    uint32 InstanceUID = 0;

    bool operator==(const FActorInstanceHandle& Other) const = default;
    bool IsValid() const { return Actor.IsValid() || ReferenceObject != nullptr; }
    bool DoesRepresentClass(const UClass* OtherClass) const;
};

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

USTRUCT()
struct FResponseChannel {
    GENERATED_BODY()
    UPROPERTY()
    FName Channel;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> Response = ECR_Block;
};

USTRUCT()
struct FCollisionResponseContainer {
    GENERATED_BODY()
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> WorldStatic = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> WorldDynamic = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> Pawn = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> Visibility = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> Camera = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> PhysicsBody = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> Vehicle = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> Destructible = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> EngineTraceChannel1 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> EngineTraceChannel2 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> EngineTraceChannel3 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> EngineTraceChannel4 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> EngineTraceChannel5 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> EngineTraceChannel6 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel1 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel2 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel3 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel4 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel5 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel6 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel7 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel8 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel9 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel10 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel11 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel12 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel13 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel14 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel15 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel16 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel17 = ECR_Block;
    UPROPERTY()
    TEnumAsByte<ECollisionResponse> GameTraceChannel18 = ECR_Block;
};

UENUM()
enum EWalkableSlopeBehavior : int32 { WalkableSlope_Default, WalkableSlope_Increase, WalkableSlope_Decrease, WalkableSlope_Unwalkable, WalkableSlope_Max };

USTRUCT()
struct FWalkableSlopeOverride {
    GENERATED_BODY()
    UPROPERTY()
    TEnumAsByte<EWalkableSlopeBehavior> WalkableSlopeBehavior = WalkableSlope_Default;
    UPROPERTY()
    float WalkableSlopeAngle = 0.0f;
};

UENUM()
enum ETriangleSortOption : int32 {
    TRISORT_None,
    TRISORT_CenterRadialDistance,
    TRISORT_Random,
    TRISORT_MergeContiguous,
    TRISORT_Custom,
    TRISORT_CustomLeftRight,
    TRISORT_MAX
};

UENUM()
enum ETriangleSortAxis : int32 { TSA_X_Axis, TSA_Y_Axis, TSA_Z_Axis, TSA_MAX };

USTRUCT()
struct FSkeletalMeshBuildSettings {
    GENERATED_BODY()
    UPROPERTY()
    bool bRecomputeNormals = true;
    UPROPERTY()
    bool bRecomputeTangents = true;
    UPROPERTY()
    bool bUseMikkTSpace = true;
    UPROPERTY()
    bool bComputeWeightedNormals = false;
    UPROPERTY()
    bool bRemoveDegenerates = true;
    UPROPERTY()
    bool bUseHighPrecisionTangentBasis = false;
    UPROPERTY()
    bool bUseHighPrecisionSkinWeights = false;
    UPROPERTY()
    bool bUseFullPrecisionUVs = false;
    UPROPERTY()
    bool bUseBackwardsCompatibleF16TruncUVs = false;
    UPROPERTY()
    bool bOptimizeForInstancing = false;
    UPROPERTY()
    bool bBuildAdjacencyBuffer = true;
    UPROPERTY()
    float ThresholdPosition = 0.00002f;
    UPROPERTY()
    float ThresholdTangentNormal = 0.00002f;
    UPROPERTY()
    float ThresholdUV = 0.0009765625f;
    UPROPERTY()
    float MorphThresholdPosition = 0.015f;
    UPROPERTY()
    int32 BoneInfluenceLimit = 0;
};

UENUM()
enum class ENaniteFallbackTarget : uint8 { Auto, PercentTriangles, RelativeError };

UENUM()
enum class ENaniteGenerateFallback : uint8 { PlatformDefault, Enabled };

UENUM()
enum class ENaniteShapePreservation : uint8 { None, PreserveArea, Voxelize };

USTRUCT()
struct FMeshDisplacementMap {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UTexture2D> Texture = nullptr;
    UPROPERTY()
    float Magnitude = 0.0f;
    UPROPERTY()
    float Center = 0.0f;
};

USTRUCT()
struct FMeshNaniteSettings {
    GENERATED_BODY()
    UPROPERTY()
    bool bEnabled = false;
    UPROPERTY()
    bool bPreserveArea = false;
    UPROPERTY()
    bool bExplicitTangents = false;
    UPROPERTY()
    bool bLerpUVs = true;
    UPROPERTY()
    bool bSeparable = false;
    UPROPERTY()
    bool bVoxelNDF = true;
    UPROPERTY()
    bool bVoxelOpacity = false;
    UPROPERTY()
    ENaniteShapePreservation ShapePreservation = ENaniteShapePreservation::None;
    UPROPERTY()
    int32 PositionPrecision = UE_MIN_int32;
    UPROPERTY()
    int32 NormalPrecision = -1;
    UPROPERTY()
    int32 TangentPrecision = -1;
    UPROPERTY()
    int32 BoneWeightPrecision = -1;
    UPROPERTY()
    uint32 TargetMinimumResidencyInKB = 0;
    UPROPERTY()
    float KeepPercentTriangles = 1.0f;
    UPROPERTY()
    float TrimRelativeError = 0.0f;
    UPROPERTY()
    ENaniteGenerateFallback GenerateFallback = ENaniteGenerateFallback::PlatformDefault;
    UPROPERTY()
    ENaniteFallbackTarget FallbackTarget = ENaniteFallbackTarget::Auto;
    UPROPERTY()
    float FallbackPercentTriangles = 1.0f;
    UPROPERTY()
    float FallbackRelativeError = 1.0f;
    UPROPERTY()
    float MaxEdgeLengthFactor = 0.0f;
    UPROPERTY()
    int32 NumRays = 64;
    UPROPERTY()
    int32 VoxelLevel = 0;
    UPROPERTY()
    float RayBackUp = 0.0f;
    UPROPERTY()
    int32 DisplacementUVChannel = 0;
    UPROPERTY()
    TArray<FMeshDisplacementMap> DisplacementMaps;
    UPROPERTY()
    FNaniteAssemblyData NaniteAssemblyData;
};

USTRUCT()
struct FSubtitleCue {
    GENERATED_BODY()
    UPROPERTY()
    FText Text;
    UPROPERTY()
    float Time = 0.0f;
};

USTRUCT()
struct FLocalizedSubtitle {
    GENERATED_BODY()
    UPROPERTY()
    FString LanguageExt;
    UPROPERTY()
    TArray<FSubtitleCue> Subtitles;
    UPROPERTY()
    uint32 bMature : 1 = 0;
    UPROPERTY()
    uint32 bManualWordWrap : 1 = 0;
    UPROPERTY()
    uint32 bSingleLine : 1 = 0;
};


USTRUCT()
struct FLightmassPrimitiveSettings {
    GENERATED_BODY()
    UPROPERTY()
    uint32 bUseTwoSidedLighting : 1 = 0;
    UPROPERTY()
    uint32 bShadowIndirectOnly : 1 = 0;
    UPROPERTY()
    uint32 bUseEmissiveForStaticLighting : 1 = 0;
    UPROPERTY()
    uint32 bUseVertexNormalForHemisphereGather : 1 = 0;
    UPROPERTY()
    float EmissiveLightFalloffExponent = 8.0f;
    UPROPERTY()
    float EmissiveLightExplicitInfluenceRadius = 0.0f;
    UPROPERTY()
    float EmissiveBoost = 1.0f;
    UPROPERTY()
    float DiffuseBoost = 1.0f;
    UPROPERTY()
    float FullyOccludedSamplesFraction = 1.0f;

    friend FArchive& operator<<(FArchive& Ar, FLightmassPrimitiveSettings& Settings);
};

USTRUCT()
struct FLightmassLightSettings {
    GENERATED_BODY()
    UPROPERTY()
    float IndirectLightingSaturation = 1.0f;
    UPROPERTY()
    float ShadowExponent = 2.0f;
    UPROPERTY()
    bool bUseAreaShadowsForStationaryLight = false;
};

USTRUCT()
struct FLightmassPointLightSettings : public FLightmassLightSettings {
    GENERATED_BODY()
};

USTRUCT()
struct FLightmassDirectionalLightSettings : public FLightmassLightSettings {
    GENERATED_BODY()
    UPROPERTY()
    float LightSourceAngle = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FConstraintBrokenSignature, int32, ConstraintIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPlasticDeformationEventSignature, int32, ConstraintIndex);

}
