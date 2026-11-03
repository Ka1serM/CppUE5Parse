#pragma once

#include "ue5parse/runtime/Core/Math/Color.h"
#include "ue5parse/runtime/Core/Math/RandomStream.h"
#include "ue5parse/runtime/CoreUObject/UObject/WeakObjectPtrTemplates.h"
#include "ue5parse/runtime/Engine/Components/PrimitiveComponent.h"
#include "ue5parse/runtime/Engine/Particles/ParticleSystem.h"
#include "ue5parse/runtime/Engine/Particles/WorldPSCPool.h"

#include "ParticleSystemComponent.generated.h"

namespace ue5parse::runtime {

class AActor;
class UMaterialInterface;
class UParticleModuleEventSendToGame;
class UPhysicalMaterial;
class USkeletalMeshComponent;
struct FParticleEmitterInstance;

UENUM()
enum EParticleSysParamType : int32 {
    PSPT_None,
    PSPT_Scalar,
    PSPT_ScalarRand,
    PSPT_Vector,
    PSPT_VectorRand,
    PSPT_Color,
    PSPT_Actor,
    PSPT_Material,
    PSPT_VectorUnitRand,
    PSPT_MAX,
};

// Riot's: the skin variant whose color the AresParticleModuleVariantColor modules take.
UENUM()
enum class EAresParticleVariantColor : uint8 {
    AresVariantBaseColor,
    AresVariant1Color,
    AresVariant2Color,
    AresVariant3Color,
    AresVariant4Color,
};

// FParticleSysParam: a named instance parameter that the distributions and modules of the template read (UE's
// Particles/ParticleSystemComponent.h).
USTRUCT()
struct FParticleSysParam {
    GENERATED_BODY()
    UPROPERTY()
    FName Name;
    UPROPERTY()
    TEnumAsByte<EParticleSysParamType> ParamType = PSPT_None;
    UPROPERTY()
    float Scalar = 0;
    UPROPERTY()
    float Scalar_Low = 0;
    UPROPERTY()
    FVector Vector = FVector::ZeroVector;
    UPROPERTY()
    FVector Vector_Low = FVector::ZeroVector;
    UPROPERTY()
    FColor Color = FColor(0, 0, 0, 0);
    UPROPERTY()
    TObjectPtr<AActor> Actor = nullptr;
    UPROPERTY()
    TObjectPtr<UMaterialInterface> Material = nullptr;
};

struct FParticleEventData {
    int32 Type = 0;
    FName EventName;
    float EmitterTime = 0;
    FVector Location = FVector::ZeroVector;
    FVector Velocity = FVector::ZeroVector;
    TArray<TObjectPtr<UParticleModuleEventSendToGame>> EventData;
};

struct FParticleExistingData : FParticleEventData {
    float ParticleTime = 0;
    FVector Direction = FVector::ZeroVector;
};

struct FParticleEventSpawnData : FParticleEventData {};

struct FParticleEventDeathData : FParticleExistingData {};

struct FParticleEventCollideData : FParticleExistingData {
    FVector Normal = FVector::ZeroVector;
    float Time = 0;
    int32 Item = 0;
    FName BoneName;
    UPhysicalMaterial* PhysMat = nullptr;
};

struct FParticleEventBurstData : FParticleEventData {
    int32 ParticleCount = 0;
};

struct FParticleEventKismetData : FParticleEventData {};

// UFXSystemComponent: the base of the particle system components of Cascade and Niagara (UE's
// Particles/ParticleSystemComponent.h).
UCLASS(abstract)
class UFXSystemComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable)
    virtual void SetFloatParameter(FName ParameterName, float Param) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetVectorParameter(FName ParameterName, FVector Param) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetColorParameter(FName ParameterName, FLinearColor Param) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetActorParameter(FName ParameterName, AActor* Param) {}
    UFUNCTION(BlueprintCallable)
    virtual void SetEmitterEnable(FName EmitterName, bool bNewEnableState) {}
    UFUNCTION(BlueprintCallable)
    virtual void ReleaseToPool() {}
    virtual UFXSystemAsset* GetFXSystemAsset() const { return nullptr; }
};

// UParticleSystemComponent: plays a Cascade particle system (UE's Particles/ParticleSystemComponent.h). This is the
// game thread simulation of UE: one emitter instance per emitter, ticked in the component tick. The export records
// the particles of the instances; the render side of UE is not ported.
UCLASS()
class UParticleSystemComponent : public UFXSystemComponent {
    GENERATED_BODY()
public:
    explicit UParticleSystemComponent(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UParticleSystem> Template = nullptr;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInterface>> EmitterMaterials;
    UPROPERTY()
    TArray<TObjectPtr<USkeletalMeshComponent>> SkelMeshComponents;
    UPROPERTY()
    FVector PatchCenter = FVector::ZeroVector;
    UPROPERTY()
    FVector2D PatchRadiusAndHeight = FVector2D(0, 0);
    UPROPERTY()
    bool bResetOnDetach = false;
    UPROPERTY()
    bool bUpdateOnDedicatedServer = false;
    UPROPERTY()
    bool bAllowRecycling = false;
    UPROPERTY()
    bool bAutoManageAttachment = false;
    UPROPERTY()
    bool bAutoAttachWeldSimulatedBodies = false;
    UPROPERTY()
    bool bWarmingUp = false;
    UPROPERTY()
    bool bOverrideLODMethod = false;
    UPROPERTY()
    bool bSkipUpdateDynamicDataDuringTick = false;
    UPROPERTY()
    TEnumAsByte<ParticleSystemLODMethod> LODMethod = PARTICLESYSTEMLODMETHOD_Automatic;
    UPROPERTY()
    EParticleSignificanceLevel RequiredSignificance = EParticleSignificanceLevel::Low;
    UPROPERTY()
    EAresParticleVariantColor ParticleVariantColor = EAresParticleVariantColor::AresVariantBaseColor;
    UPROPERTY()
    FName LUTVariantParameter;
    // Riot's: a custom color module colors a named material slot, so Tick_MaterialOverrides colors it again.
    bool bCustomColorOnNamedMaterials = false;
    UPROPERTY()
    TArray<FParticleSysParam> InstanceParameters;
    UPROPERTY()
    TArray<FTransform> PatchGrid;
    UPROPERTY()
    FMulticastScriptDelegate OnParticleSpawn;
    UPROPERTY()
    FMulticastScriptDelegate OnParticleBurst;
    UPROPERTY()
    FMulticastScriptDelegate OnParticleDeath;
    UPROPERTY()
    FMulticastScriptDelegate OnParticleCollide;
    UPROPERTY()
    bool bOldPositionValid = false;
    UPROPERTY()
    FVector OldPosition = FVector::ZeroVector;
    UPROPERTY()
    FVector PartSysVelocity = FVector::ZeroVector;
    UPROPERTY()
    float WarmupTime = 0;
    UPROPERTY()
    float AdditionalWarmupTime = 0;
    UPROPERTY()
    float WarmupTickRate = 0;
    UPROPERTY()
    float SecondsBeforeInactive = 1.0f;
    UPROPERTY()
    float MaxTimeBeforeForceUpdateTransform = 5.0f;
    UPROPERTY()
    TArray<TObjectPtr<UObject>> ReplayClips;
    UPROPERTY()
    float CustomTimeDilation = 1.0f;
    UPROPERTY()
    TWeakObjectPtr<USceneComponent> AutoAttachParent;
    UPROPERTY()
    FName AutoAttachSocketName;
    UPROPERTY()
    EAttachmentRule AutoAttachLocationRule = EAttachmentRule::KeepRelative;
    UPROPERTY()
    EAttachmentRule AutoAttachRotationRule = EAttachmentRule::KeepRelative;
    UPROPERTY()
    EAttachmentRule AutoAttachScaleRule = EAttachmentRule::KeepRelative;
    UPROPERTY()
    FMulticastScriptDelegate OnSystemFinished;

    TArray<FParticleEmitterInstance*> EmitterInstances;
    FRandomStream RandomStream;
    TArray<FParticleEventSpawnData> SpawnEvents;
    TArray<FParticleEventDeathData> DeathEvents;
    TArray<FParticleEventCollideData> CollisionEvents;
    TArray<FParticleEventBurstData> BurstEvents;
    TArray<FParticleEventKismetData> KismetEvents;
    EPSCPoolMethod PoolingMethod = EPSCPoolMethod::None;
    int32 LODLevel = 0;
    int32 TotalActiveParticles = 0;
    uint32 NumSignificantEmitters = 0;
    uint32 TimeSinceLastTick = 0;
    float DeltaTimeTick = 0;
    double AccumTickTime = 0;
    float AccumLODDistanceCheckTime = 0;
    float LastSignificantTime = 0;
    float EmitterDelay = 0;
    FVector SavedAutoAttachRelativeLocation = FVector::ZeroVector;
    FRotator SavedAutoAttachRelativeRotation = FRotator::ZeroRotator;
    FVector SavedAutoAttachRelativeScale3D = FVector(1, 1, 1);
    bool bWasCompleted = false;
    bool bSuppressSpawning = false;
    bool bWasDeactivated = false;
    bool bWasActive = false;
    bool bResetTriggered = false;
    bool bDeactivateTriggered = false;
    bool bJustRegistered = false;
    bool bHasBeenActivated = false;
    bool bIsManagingSignificance = false;
    bool bWasManagingSignificance = false;
    bool bIsDuringRegister = false;
    bool bAutoDestroy = false;
    bool bDidAutoAttach = false;
    // The view locations of the local player controllers and their LOD distance factors, gathered each tick.
    TArray<FVector> PlayerLocations;
    TArray<float> PlayerLODDistanceFactor;
    // The world bounds of the particles (UE's Bounds, from CalcBounds).
    FBox Bounds;

    void PostLoad() override;
    void BeginDestroy() override;
    void OnRegister() override;
    void OnUnregister() override;
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    void Activate(bool bReset = false) override;
    void Deactivate() override;
    bool ShouldActivate() const override;
    UFXSystemAsset* GetFXSystemAsset() const override { return Template; }

    UFUNCTION(BlueprintCallable)
    void SetTemplate(UParticleSystem* NewTemplate);
    UFUNCTION(BlueprintCallable)
    void ActivateSystem(bool bFlagAsJustAttached = false);
    UFUNCTION(BlueprintCallable)
    void DeactivateSystem();
    UFUNCTION(BlueprintCallable)
    void DeactivateImmediate() { Complete(); }
    UFUNCTION(BlueprintCallable)
    void KillParticlesForced();
    UFUNCTION(BlueprintCallable)
    int32 GetNumActiveParticles() const;
    UFUNCTION(BlueprintCallable)
    void SetEmitterEnable(FName EmitterName, bool bNewEnableState) override;
    UFUNCTION(BlueprintCallable)
    void SetFloatParameter(FName ParameterName, float Param) override;
    UFUNCTION(BlueprintCallable)
    void SetFloatRandParameter(FName ParameterName, float Param, float ParamLow);
    UFUNCTION(BlueprintCallable)
    void SetVectorParameter(FName ParameterName, FVector Param) override;
    UFUNCTION(BlueprintCallable)
    void SetVectorRandParameter(FName ParameterName, const FVector& Param, const FVector& ParamLow);
    UFUNCTION(BlueprintCallable)
    void SetColorParameter(FName ParameterName, FLinearColor Param) override;
    UFUNCTION(BlueprintCallable)
    void SetActorParameter(FName ParameterName, AActor* Param) override;
    UFUNCTION(BlueprintCallable)
    void SetMaterialParameter(FName ParameterName, UMaterialInterface* Param);
    UFUNCTION(BlueprintCallable)
    void SetBoolParameter(FName ParameterName, bool Param) { SetFloatParameter(ParameterName, Param ? 1.0f : 0.0f); }
    UFUNCTION(BlueprintCallable)
    void SetIntParameter(FName ParameterName, int32 Param) { SetFloatParameter(ParameterName, float(Param)); }
    UFUNCTION(BlueprintCallable)
    void ClearParameter(FName ParameterName, TEnumAsByte<EParticleSysParamType> ParameterType);
    UFUNCTION(BlueprintCallable)
    void SetAutoAttachParams(USceneComponent* Parent, FName SocketName, EAttachLocation::Type LocationType);
    UFUNCTION(BlueprintCallable)
    void SetAutoAttachmentParameters(USceneComponent* Parent, FName SocketName, EAttachmentRule LocationRule, EAttachmentRule RotationRule,
        EAttachmentRule ScaleRule);
    UFUNCTION(BlueprintCallable)
    UMaterialInterface* GetNamedMaterial(FName InName) const;
    UFUNCTION(BlueprintCallable)
    void SetMaterialByName(FName MaterialSlotName, UMaterialInterface* SourceMaterial) override;

    bool GetFloatParameter(const FName InName, float& OutFloat);
    bool GetVectorParameter(const FName InName, FVector& OutVector);
    bool GetAnyVectorParameter(const FName InName, FVector& OutVector);
    bool GetColorParameter(const FName InName, FLinearColor& OutColor);
    bool GetActorParameter(const FName InName, AActor*& OutActor);
    bool GetMaterialParameter(const FName InName, UMaterialInterface*& OutMaterial);
    int32 GetNamedMaterialIndex(FName InName) const;
    int32 GetNumMaterials() const override;
    UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
    void SetMaterial(int32 ElementIndex, UMaterialInterface* Material) override;

    void InitializeSystem();
    void InitParticles();
    void ResetParticles(bool bEmptyInstances = false);
    void ResetBurstLists();
    void Complete();
    bool HasCompleted();
    int32 GetCurrentDetailMode() const;
    int32 DetermineLODLevelForLocation(const FVector& EffectLocation);
    void SetLODLevel(int32 InLODLevel);
    int32 GetLODLevel() const { return LODLevel; }
    void CancelAutoAttachment(bool bDetachFromParent);
    const TArray<FVector>& GetPlayerLocations() const { return PlayerLocations; }
    const TArray<float>& GetPlayerLODDistanceFactor() const { return PlayerLODDistanceFactor; }
    // The recording renders every frame, so the component is always visible.
    double GetLastRenderTime() const;
    bool ParticleLineCheck(FHitResult& Hit, AActor* SourceActor, const FVector& End, const FVector& Start, const FVector& HalfExtent,
        const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes);

    void ReportEventSpawn(const FName InEventName, const float InEmitterTime, const FVector InLocation, const FVector InVelocity,
        const TArray<TObjectPtr<UParticleModuleEventSendToGame>>& InEventData);
    void ReportEventDeath(const FName InEventName, const float InEmitterTime, const FVector InLocation, const FVector InVelocity,
        const TArray<TObjectPtr<UParticleModuleEventSendToGame>>& InEventData, const float InParticleTime);
    void ReportEventCollision(const FName InEventName, const float InEmitterTime, const FVector InLocation, const FVector InDirection,
        const FVector InVelocity, const TArray<TObjectPtr<UParticleModuleEventSendToGame>>& InEventData, const float InParticleTime, const FVector InNormal,
        const float InTime, const int32 InItem, const FName InBoneName, UPhysicalMaterial* PhysMat);
    void ReportEventBurst(const FName InEventName, const float InEmitterTime, const int32 InParticleCount, const FVector InLocation,
        const TArray<TObjectPtr<UParticleModuleEventSendToGame>>& InEventData);

private:
    void ComputeTickComponent();
    void FinalizeTickComponent();
    void UpdatePlayerLocations();
    void OrientZAxisTowardCamera();
    // UE's CalcBounds, run after each tick.
    void UpdateBounds();
    void BroadcastDeactivated();
};

}
