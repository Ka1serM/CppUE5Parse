#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraPlatformSet.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Engine/DataAsset.h"
#include "Engine/Source/Runtime/GameplayTags/GameplayTagContainer.h"

#include "NiagaraScalabilitySettings.generated.h"

namespace ue5parse::runtime {

class UNiagaraSystemReadinessSettings;

UENUM()
enum class ENiagaraEmitterImportance : uint8 { Critical = 0, Normal = 1, Low = 2 };

USTRUCT()
struct FNiagaraSettingsProfile {
    GENERATED_BODY()

};

UENUM()
enum class ENiagaraPSOPrecachingTime : uint8 { None, OnInitialize, OnActivated, OnRenderStateCreation };

UENUM()
enum class ENiagaraSystemPSOPrecachingDelayMode : uint8 { DelayRendering, DelayActivation };

UENUM()
enum class ENiagaraSystemPSOPrecachingPriorityBoost : uint8 { Highest, High };

USTRUCT()
struct FNiagaraSystemPSOPrecacheSettings {
    GENERATED_BODY()

    UPROPERTY()
    ENiagaraSystemPSOPrecachingDelayMode DelayMode = ENiagaraSystemPSOPrecachingDelayMode::DelayRendering;
    UPROPERTY()
    ENiagaraSystemPSOPrecachingPriorityBoost PreCachePriorityBoost = ENiagaraSystemPSOPrecachingPriorityBoost::Highest;
    UPROPERTY()
    ENiagaraPSOPrecachingTime PrecachingTime = ENiagaraPSOPrecachingTime::OnInitialize;
    UPROPERTY()
    uint8 bCacheOnLoad : 1 = false;
};

USTRUCT()
struct FNiagaraReadinessSettings : public FNiagaraSettingsProfile {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraPlatformSet PlatformSet;
    UPROPERTY()
    FNiagaraSystemPSOPrecacheSettings PSOPrecacheSettings;
};

USTRUCT()
struct FNiagaraScalabiltiySettingsProfile {
    GENERATED_BODY()

};

USTRUCT()
struct FNiagaraReadinessSettingsProfile : public FNiagaraScalabiltiySettingsProfile {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraReadinessSettings> Settings;
};

UCLASS()
class UNiagaraSettingsProfileSet : public UDataAsset {
    GENERATED_BODY()
public:
    explicit UNiagaraSettingsProfileSet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

};

UCLASS()
class UNiagaraSystemReadinessSettings : public UNiagaraSettingsProfileSet {
    GENERATED_BODY()
public:
    explicit UNiagaraSystemReadinessSettings(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UNiagaraSystemReadinessSettings> Parent = nullptr;
    UPROPERTY()
    TMap<FGameplayTag, FNiagaraReadinessSettingsProfile> Profiles;
};

}
