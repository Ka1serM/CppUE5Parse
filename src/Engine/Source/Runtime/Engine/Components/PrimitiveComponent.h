#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/AI/Navigation/NavRelevantInterface.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineBaseTypes.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineTypes.h"
#include "Engine/Source/Runtime/Engine/HLOD/HLODBatchingPolicy.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/BodyInstance.h"
#include "Engine/Source/Runtime/Engine/SceneTypes.h"
#include "Engine/Source/Runtime/Engine/VT/RuntimeVirtualTextureEnum.h"
#include "Engine/Source/Runtime/InputCore/InputCoreTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LazyObjectPtr.h"

#include "PrimitiveComponent.generated.h"

namespace ue5parse::runtime {

class APlayerController;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPhysicalMaterial;
class UPrimitiveComponent;
class URuntimeVirtualTexture;

UENUM()
enum ECanBeCharacterBase : int32 { ECB_No, ECB_Yes, ECB_Owner, ECB_MAX };

UENUM()
enum class ERendererStencilMask : uint8 { ERSM_Default, ERSM_255, ERSM_1, ERSM_2, ERSM_4, ERSM_8, ERSM_16, ERSM_32, ERSM_64, ERSM_128 };

UENUM()
enum class ERayTracingGroupCullingPriority : uint8 { CP_0_NEVER_CULL, CP_1, CP_2, CP_3, CP_4_DEFAULT, CP_5, CP_6, CP_7, CP_8_QUICKLY_CULL };

UENUM()
enum class EFirstPersonPrimitiveType : uint8 { None, FirstPerson, WorldSpaceRepresentation };

UENUM()
enum class EComponentPhysicsStateChange : uint8 { Created, Destroyed };

DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FiveParams(FComponentHitSignature, UPrimitiveComponent, OnComponentHit, UPrimitiveComponent*, HitComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, FVector, NormalImpulse, const FHitResult&, Hit);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_SixParams(FComponentBeginOverlapSignature, UPrimitiveComponent, OnComponentBeginOverlap, UPrimitiveComponent*, OverlappedComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex, bool, bFromSweep, const FHitResult&, SweepResult);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FourParams(FComponentEndOverlapSignature, UPrimitiveComponent, OnComponentEndOverlap, UPrimitiveComponent*, OverlappedComponent, AActor*, OtherActor, UPrimitiveComponent*, OtherComp, int32, OtherBodyIndex);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FComponentWakeSignature, UPrimitiveComponent, OnComponentWake, UPrimitiveComponent*, WakingComponent, FName, BoneName);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FComponentSleepSignature, UPrimitiveComponent, OnComponentSleep, UPrimitiveComponent*, SleepingComponent, FName, BoneName);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FComponentPhysicsStateChanged, UPrimitiveComponent, OnComponentPhysicsStateChanged, UPrimitiveComponent*, ChangedComponent, EComponentPhysicsStateChange, StateChange);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FComponentBeginCursorOverSignature, UPrimitiveComponent, OnBeginCursorOver, UPrimitiveComponent*, TouchedComponent);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_OneParam(FComponentEndCursorOverSignature, UPrimitiveComponent, OnEndCursorOver, UPrimitiveComponent*, TouchedComponent);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FComponentOnClickedSignature, UPrimitiveComponent, OnClicked, UPrimitiveComponent*, TouchedComponent, FKey, ButtonPressed);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FComponentOnReleasedSignature, UPrimitiveComponent, OnReleased, UPrimitiveComponent*, TouchedComponent, FKey, ButtonReleased);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FourParams(FComponentOnTouchBeganSignature, UPrimitiveComponent, OnTouchBegan, const FTouchId, TouchId, APlayerController*, Controller, const FHitResult&, HitResult, UPrimitiveComponent*, TouchedComponent);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FourParams(FComponentOnTouchEndedSignature, UPrimitiveComponent, OnTouchEnded, const FTouchId, TouchId, APlayerController*, Controller, const FHitResult&, HitResult, UPrimitiveComponent*, TouchedComponent);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FourParams(FComponentOnTouchEnteredSignature, UPrimitiveComponent, OnTouchEntered, const FTouchId, TouchId, APlayerController*, Controller, const FHitResult&, HitResult, UPrimitiveComponent*, TouchedComponent);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_FourParams(FComponentOnTouchExitedSignature, UPrimitiveComponent, OnTouchExited, const FTouchId, TouchId, APlayerController*, Controller, const FHitResult&, HitResult, UPrimitiveComponent*, TouchedComponent);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FComponentOnInputTouchBeginSignature, UPrimitiveComponent, OnInputTouchBegin, ETouchIndex::Type, FingerIndex, UPrimitiveComponent*, TouchedComponent);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FComponentOnInputTouchEndSignature, UPrimitiveComponent, OnInputTouchEnd, ETouchIndex::Type, FingerIndex, UPrimitiveComponent*, TouchedComponent);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FComponentBeginTouchOverSignature, UPrimitiveComponent, OnInputTouchEnter, ETouchIndex::Type, FingerIndex, UPrimitiveComponent*, TouchedComponent);
DECLARE_DYNAMIC_MULTICAST_SPARSE_DELEGATE_TwoParams(FComponentEndTouchOverSignature, UPrimitiveComponent, OnInputTouchLeave, ETouchIndex::Type, FingerIndex, UPrimitiveComponent*, TouchedComponent);

UCLASS(Abstract)
class UPrimitiveComponent : public USceneComponent {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;

    UPROPERTY()
    bool bIsFirstPerson = false;
    UPROPERTY()
    bool bRenderInMainPass = true;
    UPROPERTY()
    bool CastShadow = false;
    UPROPERTY(Transient)
    TArray<TObjectPtr<AActor>> MoveIgnoreActors;
    UPROPERTY()
    FPrimitiveComponentPostPhysicsTickFunction PostPhysicsComponentTick;
    UPROPERTY()
    TLazyObjectPtr<UPrimitiveComponent> ReplacementPrimitive;
    UPROPERTY()
    float MinDrawDistance = 0.0f;
    UPROPERTY()
    float LDMaxDrawDistance = 0.f;
    UPROPERTY()
    float CachedMaxDrawDistance = 0.f;
    UPROPERTY()
    TEnumAsByte<ESceneDepthPriorityGroup> DepthPriorityGroup = SDPG_World;
    UPROPERTY()
    TEnumAsByte<ESceneDepthPriorityGroup> ViewOwnerDepthPriorityGroup = SDPG_World;
    UPROPERTY()
    uint8 bNoEncroachCheck_DEPRECATED : 1 = 0;
    UPROPERTY()
    uint8 bDisableAllRigidBody_DEPRECATED : 1 = 0;
    UPROPERTY()
    TEnumAsByte<EIndirectLightingCacheQuality> IndirectLightingCacheQuality = ILCQ_Point;
    UPROPERTY()
    ELightmapType LightmapType = ELightmapType::Default;
    UPROPERTY()
    EHLODBatchingPolicy HLODBatchingPolicy = EHLODBatchingPolicy::None;
    UPROPERTY()
    EShadowCacheInvalidationBehavior ShadowCacheInvalidationBehavior = EShadowCacheInvalidationBehavior::Auto;
    UPROPERTY()
    uint8 bEnableAutoLODGeneration : 1 = true;
    UPROPERTY()
    uint8 bIsActorTextureStreamingBuiltData : 1 = 0;
    UPROPERTY()
    uint8 bIsValidTextureStreamingBuiltData : 1 = false;
    UPROPERTY()
    uint8 bNeverDistanceCull : 1 = false;
    UPROPERTY()
    uint8 bAlwaysCreatePhysicsState : 1 = false;
    UPROPERTY()
    uint8 bGenerateOverlapEvents : 1 = true;
    UPROPERTY()
    uint8 bMultiBodyOverlap : 1 = false;
    UPROPERTY()
    uint8 bCheckAsyncSceneOnMove : 1 = 0;
    UPROPERTY()
    uint8 bTraceComplexOnMove : 1 = 0;
    UPROPERTY()
    uint8 bReturnMaterialOnMove : 1 = false;
    UPROPERTY()
    uint8 bUseViewOwnerDepthPriorityGroup : 1 = 0;
    UPROPERTY()
    uint8 bAllowCullDistanceVolume : 1 = true;
    UPROPERTY()
    uint8 bHasMotionBlurVelocityMeshes : 1 = 0;
    UPROPERTY()
    uint8 bVisibleInReflectionCaptures : 1 = true;
    UPROPERTY()
    uint8 bVisibleInRealTimeSkyCaptures : 1 = true;
    UPROPERTY()
    uint8 bVisibleInRayTracing : 1 = true;
    UPROPERTY()
    uint8 bVisibleInReflections : 1 = true;
    UPROPERTY()
    uint8 HiddenGame_DEPRECATED : 1 = 0;
    UPROPERTY()
    uint8 DrawInGame_DEPRECATED : 1 = 0;
    UPROPERTY()
    uint8 bRenderInDepthPass : 1 = true;
    UPROPERTY()
    uint8 bReceivesDecals : 1 = true;
    UPROPERTY()
    uint8 bHoldout : 1 = 0;
    UPROPERTY()
    uint8 bOwnerNoSee : 1 = 0;
    UPROPERTY()
    uint8 bOnlyOwnerSee : 1 = 0;
    UPROPERTY()
    uint8 bTreatAsBackgroundForOcclusion : 1 = 0;
    UPROPERTY()
    uint8 bUseAsOccluder : 1 = false;
    UPROPERTY()
    uint8 bSelectable : 1 = true;
    UPROPERTY()
    uint8 bWantsSelectionOutline : 1 = true;
    UPROPERTY()
    uint8 bWantsEditorEffects : 1 = false;
    UPROPERTY()
    uint8 bForceMipStreaming : 1 = 0;
    UPROPERTY()
    uint8 bHasPerInstanceHitProxies : 1 = 0;
    UPROPERTY()
    uint8 bEmissiveLightSource : 1 = false;
    UPROPERTY()
    uint8 bAffectDynamicIndirectLighting : 1 = true;
    UPROPERTY()
    uint8 bAffectIndirectLightingWhileHidden : 1 = 0;
    UPROPERTY()
    uint8 bAffectDistanceFieldLighting : 1 = true;
    UPROPERTY()
    uint8 bCastDynamicShadow : 1 = true;
    UPROPERTY()
    uint8 bCastStaticShadow : 1 = true;
    UPROPERTY()
    uint8 bCastVolumetricTranslucentShadow : 1 = false;
    UPROPERTY()
    uint8 bCastContactShadow : 1 = true;
    UPROPERTY()
    uint8 bSelfShadowOnly : 1 = 0;
    UPROPERTY()
    uint8 bCastFarShadow : 1 = 0;
    UPROPERTY()
    uint8 bCastInsetShadow : 1 = 0;
    UPROPERTY()
    uint8 bCastCinematicShadow : 1 = 0;
    UPROPERTY()
    uint8 bCastHiddenShadow : 1 = 0;
    UPROPERTY()
    uint8 bCastShadowAsTwoSided : 1 = 0;
    UPROPERTY()
    uint8 bLightAsIfStatic_DEPRECATED : 1 = 0;
    UPROPERTY()
    uint8 bLightAttachmentsAsGroup : 1 = 0;
    UPROPERTY()
    bool bHasCachedStaticLighting = false;
    UPROPERTY()
    uint8 bExcludeFromLightAttachmentGroup : 1 = 0;
    UPROPERTY()
    uint8 bReceiveMobileCSMShadows : 1 = true;
    UPROPERTY()
    uint8 bSingleSampleShadowFromStationaryLights : 1 = 0;
    UPROPERTY()
    uint8 bIgnoreRadialImpulse : 1 = 0;
    UPROPERTY()
    uint8 bIgnoreRadialForce : 1 = 0;
    UPROPERTY()
    uint8 bApplyImpulseOnDamage : 1 = true;
    UPROPERTY()
    uint8 bReplicatePhysicsToAutonomousProxy : 1 = true;
    UPROPERTY()
    uint8 bFillCollisionUnderneathForNavmesh : 1 = false;
    UPROPERTY()
    uint8 bRasterizeAsFilledConvexVolume : 1 = false;
    UPROPERTY()
    uint8 AlwaysLoadOnClient : 1 = true;
    UPROPERTY()
    uint8 AlwaysLoadOnServer : 1 = true;
    UPROPERTY()
    uint8 bAllowShadowFade : 1 = 0;
    UPROPERTY()
    uint8 bUseEditorCompositing : 1 = false;
    UPROPERTY()
    uint8 bRenderCustomDepth : 1 = 0;
    UPROPERTY()
    uint8 bVisibleInSceneCaptureOnly : 1 = false;
    UPROPERTY()
    uint8 bHiddenInSceneCapture : 1 = false;
    UPROPERTY()
    uint8 bRayTracingFarField : 1 = false;
    UPROPERTY()
    uint8 bLumenHeightfield : 1 = false;
    UPROPERTY()
    EFirstPersonPrimitiveType FirstPersonPrimitiveType = EFirstPersonPrimitiveType::None;
    UPROPERTY()
    uint8 bHasNoStreamableTextures : 1 = 0;
    UPROPERTY()
    uint8 bStaticWhenNotMoveable : 1 = true;
    UPROPERTY()
    TEnumAsByte<EHasCustomNavigableGeometry::Type> bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::No;
    UPROPERTY()
    TEnumAsByte<ECanBeCharacterBase> CanCharacterStepUpOn = ECB_Yes;
    UPROPERTY()
    FLightingChannels LightingChannels;
    UPROPERTY()
    uint8 bReceiveCombinedCSMAndStaticShadowsFromStationaryLights : 1 = 0;
    UPROPERTY()
    ERayTracingGroupCullingPriority RayTracingGroupCullingPriority = ERayTracingGroupCullingPriority::CP_4_DEFAULT;
    UPROPERTY()
    ERendererStencilMask CustomDepthStencilWriteMask = ERendererStencilMask::ERSM_Default;
    UPROPERTY()
    uint8 ExcludeFromHLODLevels = 0;
    UPROPERTY()
    int32 RayTracingGroupId = -1;
    UPROPERTY()
    int32 VisibilityId = INDEX_NONE;
    UPROPERTY()
    float LpvBiasMultiplier = 0;
    UPROPERTY()
    int32 CustomDepthStencilValue = 0;
    UPROPERTY()
    FCustomPrimitiveData CustomPrimitiveData;
    UPROPERTY()
    int32 TranslucencySortPriority = 0;
    UPROPERTY()
    float TranslucencySortDistanceOffset = 0.0f;
    UPROPERTY()
    TArray<TObjectPtr<URuntimeVirtualTexture>> RuntimeVirtualTextures;
    UPROPERTY()
    int8 VirtualTextureLodBias = 0;
    UPROPERTY()
    int8 VirtualTextureCullMips = 0;
    UPROPERTY()
    int8 VirtualTextureMinCoverage = 0;
    UPROPERTY()
    ERuntimeVirtualTextureMainPassType VirtualTextureRenderPassType = ERuntimeVirtualTextureMainPassType::Exclusive;
    UPROPERTY()
    float BoundsScale = 1.0f;
    UPROPERTY()
    uint8 bRenderInMono : 1 = 0;
    UPROPERTY()
    FBodyInstance BodyInstance;
    UPROPERTY()
    FComponentHitSignature OnComponentHit;
    UPROPERTY()
    FComponentBeginOverlapSignature OnComponentBeginOverlap;
    UPROPERTY()
    FComponentEndOverlapSignature OnComponentEndOverlap;
    UPROPERTY()
    FComponentWakeSignature OnComponentWake;
    UPROPERTY()
    FComponentSleepSignature OnComponentSleep;
    UPROPERTY()
    FComponentPhysicsStateChanged OnComponentPhysicsStateChanged;
    UPROPERTY()
    FComponentBeginCursorOverSignature OnBeginCursorOver;
    UPROPERTY()
    FComponentEndCursorOverSignature OnEndCursorOver;
    UPROPERTY()
    FComponentOnClickedSignature OnClicked;
    UPROPERTY()
    FComponentOnReleasedSignature OnReleased;
    UPROPERTY()
    FComponentOnInputTouchBeginSignature OnInputTouchBegin;
    UPROPERTY()
    FComponentOnInputTouchEndSignature OnInputTouchEnd;
    UPROPERTY()
    FComponentBeginTouchOverSignature OnInputTouchEnter;
    UPROPERTY()
    FComponentEndTouchOverSignature OnInputTouchLeave;
    UPROPERTY()
    FComponentOnTouchBeganSignature OnTouchBegan;
    UPROPERTY()
    FComponentOnTouchEndedSignature OnTouchEnded;
    UPROPERTY()
    FComponentOnTouchEnteredSignature OnTouchEntered;
    UPROPERTY()
    FComponentOnTouchExitedSignature OnTouchExited;
    UPROPERTY()
    TObjectPtr<UPrimitiveComponent> LODParentPrimitive = nullptr;

    explicit UPrimitiveComponent(const FObjectInitializer& ObjectInitializer);
    UFUNCTION(BlueprintCallable)
    virtual int32 GetNumMaterials() const { return 0; }
    UFUNCTION(BlueprintPure)
    virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const { return nullptr; }
    UFUNCTION(BlueprintCallable)
    virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* Material) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetMaterialByName(FName MaterialSlotName, UMaterialInterface* Material) {}
    UFUNCTION(BlueprintCallable)
    virtual UMaterialInstanceDynamic* CreateDynamicMaterialInstance(int32 ElementIndex, UMaterialInterface* SourceMaterial = nullptr,
                                                                    FName OptionalName = FName());
    UFUNCTION(BlueprintCallable)
    virtual void PrestreamTextures(float Seconds, bool bPrioritizeCharacterTextures, int32 CinematicTextureGroups) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetCollisionEnabled(TEnumAsByte<ECollisionEnabled::Type> NewType) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetPhysMaterialOverride(UPhysicalMaterial* NewPhysMaterial) {}
    UFUNCTION(BlueprintCallable)
    void SetTranslucentSortPriority(int32 NewTranslucentSortPriority) {}
    void SetCastShadow(bool NewCastShadow) { CastShadow = NewCastShadow; }
    UFUNCTION(BlueprintCallable)
    void SetCollisionResponseToChannel(TEnumAsByte<ECollisionChannel> Channel, TEnumAsByte<ECollisionResponse> NewResponse) {}
    bool IsAnySimulatingPhysics() const { return false; }
    void AddImpulseAtLocation(FVector Impulse, FVector Location, FName BoneName = NAME_None) {}
};

}
