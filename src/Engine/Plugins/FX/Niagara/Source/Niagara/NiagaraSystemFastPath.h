#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"

#include "NiagaraSystemFastPath.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraFastPath_Module_SystemScalability {
    GENERATED_BODY()

    UPROPERTY()
    bool bUseSystemScalability = false;
    UPROPERTY()
    ENiagaraExecutionState CulledState = ENiagaraExecutionState::Active;
    UPROPERTY()
    bool bEnableVisibilityCulling = false;
    UPROPERTY()
    float VisibilityCullDelay = 0;
};

USTRUCT()
struct FNiagaraFastPath_Module_SystemLifeCycle {
    GENERATED_BODY()

    UPROPERTY()
    bool bCompleteOnInactive = false;
};

UENUM()
enum class ENiagaraFastPathMode : int32 { ScriptVMOnly, FastPathOnly, ScrptVMAndFastPath };

UENUM()
enum class ENiagaraFastPathInputType : int32 { Local, RangedSpawn, RangedUpdate, User };

USTRUCT()
struct FNiagaraFastPathFloatInput {
    GENERATED_BODY()

    UPROPERTY()
    ENiagaraFastPathInputType Type = ENiagaraFastPathInputType::Local;
    UPROPERTY()
    float Local = 0;
    UPROPERTY()
    float RangeMin = 0;
    UPROPERTY()
    float RangeMax = 0;
    UPROPERTY()
    FName UserParameterName;
};

USTRUCT()
struct FNiagaraFastPath_Module_SpawnRate {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraFastPathFloatInput SpawnRate;
    UPROPERTY()
    int32 SpawnGroup = 0;
};

USTRUCT()
struct FNiagaraFastPath_Module_EmitterScalability {
    GENERATED_BODY()

    UPROPERTY()
    bool bUseEmitterScalability = false;
    UPROPERTY()
    bool bUseMaxDistance = false;
    UPROPERTY()
    ENiagaraExecutionState MaxCulledState = ENiagaraExecutionState::Active;
    UPROPERTY()
    float MaxDistance = 0;
    UPROPERTY()
    bool bUseMinDistance = false;
    UPROPERTY()
    ENiagaraExecutionState MinCulledState = ENiagaraExecutionState::Active;
    UPROPERTY()
    float MinDistance = 0;
    UPROPERTY()
    bool bApplySpawnCountScale = false;
    UPROPERTY()
    float SpawnCountScale = 0;
    UPROPERTY()
    bool bApplySpawnCountScaleByDistanceFraction = false;
    UPROPERTY()
    float ScaleForMinFraction = 0;
    UPROPERTY()
    float ScaleForMaxFraction = 0;
};

USTRUCT()
struct FNiagaraFastPathIntInput {
    GENERATED_BODY()

    UPROPERTY()
    ENiagaraFastPathInputType Type = ENiagaraFastPathInputType::Local;
    UPROPERTY()
    int32 Local = 0;
    UPROPERTY()
    int32 RangeMin = 0;
    UPROPERTY()
    int32 RangeMax = 0;
    UPROPERTY()
    FName UserParameterName;
};

USTRUCT()
struct FNiagaraFastPath_Module_SpawnBurstInstantaneous {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraFastPathIntInput SpawnCount;
    UPROPERTY()
    FNiagaraFastPathFloatInput SpawnTime;
    UPROPERTY()
    int32 SpawnGroup = 0;
};

USTRUCT()
struct FNiagaraFastPathAttributeNames {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FName> System;
    UPROPERTY()
    TArray<FName> SystemFullNames;
    UPROPERTY()
    TArray<FName> Emitter;
    UPROPERTY()
    TArray<FName> EmitterFullNames;
};

USTRUCT()
struct FNiagaraFastPath_Module_SpawnPerUnit {
    GENERATED_BODY()

    UPROPERTY()
    FNiagaraFastPathFloatInput SpawnPerUnit;
    UPROPERTY()
    bool bUseMovementTolerance = false;
    UPROPERTY()
    float MovementTolerance = 0;
    UPROPERTY()
    bool bUseMaxMovementThreshold = false;
    UPROPERTY()
    float MaxMovementThreshold = 0;
    UPROPERTY()
    int32 SpawnGroup = 0;
};

USTRUCT()
struct FNiagaraFastPath_Module_EmitterLifeCycle {
    GENERATED_BODY()

    UPROPERTY()
    bool bAutoComplete = false;
    UPROPERTY()
    bool bCompleteOnInactive = false;
    UPROPERTY()
    bool bDelayFirstLoopOnly = false;
    UPROPERTY()
    bool bDurationRecalcEachLoop = false;
    UPROPERTY()
    int32 MaxLoopCount = 0;
    UPROPERTY()
    float NextLoopDelay = 0;
    UPROPERTY()
    float NextLoopDuration = 0;
};

}
