#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraPerfBaseline.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraPlatformSet.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/GameplayTags/GameplayTagContainer.h"

#include "NiagaraEffectType.generated.h"

namespace ue5parse::runtime {

class UNiagaraBaselineController;
class UNiagaraSignificanceHandler;
class UNiagaraSystemReadinessSettings;
class UNiagaraValidationRule;

UENUM()
enum class ENiagaraCullProxyMode : uint32 { None, Instanced_Rendered };

USTRUCT()
struct FNiagaraLinearRamp {
    GENERATED_BODY()

    UPROPERTY()
    float StartX = 0.0f;
    UPROPERTY()
    float StartY = 0.0f;
    UPROPERTY()
    float EndX = 1.0f;
    UPROPERTY()
    float EndY = 1.0f;
};

USTRUCT()
struct FNiagaraGlobalBudgetScaling {
    GENERATED_BODY()

    UPROPERTY()
    uint32 bCullByGlobalBudget : 1 = 0;
    UPROPERTY()
    uint32 bScaleMaxDistanceByGlobalBudgetUse : 1 = 0;
    UPROPERTY()
    uint32 bScaleMaxInstanceCountByGlobalBudgetUse : 1 = 0;
    UPROPERTY()
    uint32 bScaleSystemInstanceCountByGlobalBudgetUse : 1 = 0;
    UPROPERTY()
    float MaxGlobalBudgetUsage = 0;
    UPROPERTY()
    FNiagaraLinearRamp MaxDistanceScaleByGlobalBudgetUse;
    UPROPERTY()
    FNiagaraLinearRamp MaxInstanceCountScaleByGlobalBudgetUse;
    UPROPERTY()
    FNiagaraLinearRamp MaxSystemInstanceCountScaleByGlobalBudgetUse;
};

USTRUCT()
struct FNiagaraSystemVisibilityCullingSettings {
    GENERATED_BODY()

    UPROPERTY()
    uint32 bCullWhenNotRendered : 1 = 0;
    UPROPERTY()
    uint32 bCullByViewFrustum : 1 = 0;
    UPROPERTY()
    uint32 bAllowPreCullingByViewFrustum : 1 = 0;
    UPROPERTY()
    float MaxTimeOutsideViewFrustum = 0;
    UPROPERTY()
    float MaxTimeWithoutRender = 0;
};

USTRUCT()
struct FNiagaraSystemScalabilitySettings {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraPlatformSet Platforms;
    UPROPERTY()
    uint32 bCullByDistance : 1 = 0;
    UPROPERTY()
    uint32 bCullMaxInstanceCount : 1 = 0;
    UPROPERTY()
    uint32 bCullByMaxTimeWithoutRender : 1 = 0;
    UPROPERTY()
    float MaxDistance = 0;
    UPROPERTY()
    int32 MaxInstances = 0;
    UPROPERTY()
    float MaxTimeWithoutRender = 0;
    UPROPERTY()
    uint32 bCullPerSystemMaxInstanceCount : 1 = 0;
    UPROPERTY()
    int32 MaxSystemInstances = 0;
    UPROPERTY()
    uint32 bCullByGlobalBudget : 1 = 0;
    UPROPERTY()
    float MaxGlobalBudgetUsage = 0;
    UPROPERTY()
    ENiagaraCullProxyMode CullProxyMode = ENiagaraCullProxyMode::None;
    UPROPERTY()
    int32 MaxSystemProxies = 32;
    UPROPERTY()
    FNiagaraGlobalBudgetScaling BudgetScaling;
    UPROPERTY()
    FNiagaraSystemVisibilityCullingSettings VisibilityCulling;
};

USTRUCT()
struct FNiagaraSystemScalabilityOverride : public FNiagaraSystemScalabilitySettings {
    GENERATED_BODY()

    UPROPERTY()
    uint32 bOverrideDistanceSettings : 1 = 0;
    UPROPERTY()
    uint32 bOverrideInstanceCountSettings : 1 = 0;
    UPROPERTY()
    uint32 bOverrideTimeSinceRendererSettings : 1 = 0;
    UPROPERTY()
    uint32 bOverridePerSystemInstanceCountSettings : 1 = 0;
    UPROPERTY()
    uint32 bOverrideGlobalBudgetCullingSettings : 1 = 0;
    UPROPERTY()
    uint32 bOverrideGlobalBudgetScalingSettings : 1 = 0;
    UPROPERTY()
    uint32 bOverrideCullProxySettings : 1 = 0;
    UPROPERTY()
    uint32 bOverrideVisibilitySettings : 1 = 0;
};

UENUM()
enum class ENiagaraCullReaction : int32 { Deactivate, DeactivateImmediate, DeactivateResume, DeactivateImmediateResume, PauseResume };

UENUM()
enum class ENiagaraScalabilityUpdateFrequency : int32 { SpawnOnly, Low, Medium, High, Continuous };

USTRUCT()
struct FNiagaraEmitterScalabilitySettings {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraPlatformSet Platforms;
    UPROPERTY()
    uint32 bScaleSpawnCount : 1 = 0;
    UPROPERTY()
    float SpawnCountScale = 0;
};

USTRUCT()
struct FNiagaraEmitterScalabilitySettingsArray {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraEmitterScalabilitySettings> Settings;
};

USTRUCT()
struct FNiagaraSystemScalabilitySettingsArray {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraSystemScalabilitySettings> Settings;
};

UCLASS()
class UNiagaraEffectType : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraEffectType(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    ENiagaraScalabilityUpdateFrequency UpdateFrequency = ENiagaraScalabilityUpdateFrequency::SpawnOnly;
    UPROPERTY()
    ENiagaraCullReaction CullReaction = ENiagaraCullReaction::Deactivate;
    UPROPERTY()
    TArray<FNiagaraSystemScalabilitySettings> DetailLevelScalabilitySettings;
    UPROPERTY()
    FNiagaraSystemScalabilitySettingsArray SystemScalabilitySettings;
    UPROPERTY()
    FNiagaraEmitterScalabilitySettingsArray EmitterScalabilitySettings;
    UPROPERTY()
    TObjectPtr<UNiagaraSignificanceHandler> SignificanceHandler = nullptr;
    UPROPERTY()
    TObjectPtr<UNiagaraBaselineController> PerformanceBaselineController = nullptr;
    UPROPERTY()
    FNiagaraPerfBaselineStats PerfBaselineStats;
    UPROPERTY()
    FGuid PerfBaselineVersion;
    UPROPERTY()
    bool bAllowCullingForLocalPlayers = false;
    UPROPERTY()
    TArray<TObjectPtr<UNiagaraValidationRule>> ValidationRules;
    UPROPERTY()
    TObjectPtr<UNiagaraSystemReadinessSettings> ReadinessSettings = nullptr;
    UPROPERTY()
    FGameplayTag ReadinessProfile;
};

UCLASS()
class UNiagaraSignificanceHandler : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraSignificanceHandler(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

};

USTRUCT()
struct FNiagaraEmitterScalabilityOverride : public FNiagaraEmitterScalabilitySettings {
    GENERATED_BODY()

    UPROPERTY()
    uint32 bOverrideSpawnCountScale : 1 = 0;
};

USTRUCT()
struct FNiagaraEmitterScalabilityOverrides {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraEmitterScalabilityOverride> Overrides;
};

}
