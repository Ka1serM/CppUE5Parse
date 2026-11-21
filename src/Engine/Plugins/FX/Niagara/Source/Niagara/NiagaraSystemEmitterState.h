#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraStatelessDistribution.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"

#include "NiagaraSystemEmitterState.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ENiagaraLoopBehavior : uint8 { Infinite, Multiple, Once };

UENUM()
enum class ENiagaraSystemInactiveResponse : uint8 { Complete, Kill };

USTRUCT()
struct FNiagaraSystemStateData {
    GENERATED_BODY()

    UPROPERTY()
    uint32 bIgnoreSystemState : 1 = true;
    UPROPERTY()
    ENiagaraSystemInactiveResponse InactiveResponse = ENiagaraSystemInactiveResponse::Complete;
    UPROPERTY()
    ENiagaraLoopBehavior LoopBehavior = ENiagaraLoopBehavior::Once;
    UPROPERTY()
    FNiagaraDistributionRangeFloat LoopDuration;
    UPROPERTY()
    int LoopCount = 1;
    UPROPERTY()
    uint32 bRecalculateDurationEachLoop : 1 = false;
    UPROPERTY()
    FNiagaraDistributionRangeFloat LoopDelay;
    UPROPERTY()
    uint32 bDelayFirstLoopOnly : 1 = false;
    UPROPERTY()
    uint32 bRecalculateDelayEachLoop : 1 = false;
    UPROPERTY()
    uint32 bRunSpawnScript : 1 = true;
    UPROPERTY()
    uint32 bRunUpdateScript : 1 = true;
    UPROPERTY()
    uint32 bLoopDelayEnabled : 1 = false;
};

UENUM()
enum class ENiagaraEmitterInactiveResponse : uint8 { Complete, Kill };

UENUM()
enum class ENiagaraLoopDurationMode : uint8 { Fixed, Infinite };

USTRUCT()
struct FNiagaraEmitterStateData {
    GENERATED_BODY()

    UPROPERTY()
    ENiagaraEmitterInactiveResponse InactiveResponse = ENiagaraEmitterInactiveResponse::Complete;
    UPROPERTY()
    ENiagaraLoopBehavior LoopBehavior = ENiagaraLoopBehavior::Infinite;
    UPROPERTY()
    int32 LoopCount = 1;
    UPROPERTY()
    FNiagaraDistributionRangeFloat LoopDuration;
    UPROPERTY()
    uint32 bRecalculateDurationEachLoop : 1 = false;
    UPROPERTY()
    FNiagaraDistributionRangeFloat LoopDelay;
    UPROPERTY()
    uint32 bDelayFirstLoopOnly : 1 = false;
    UPROPERTY()
    uint32 bRecalculateDelayEachLoop : 1 = false;
    UPROPERTY()
    ENiagaraLoopDurationMode LoopDurationMode = ENiagaraLoopDurationMode::Fixed;
    UPROPERTY()
    uint32 bLoopDelayEnabled : 1 = false;
    UPROPERTY()
    uint32 bEnableDistanceCulling : 1 = false;
    UPROPERTY()
    uint32 bEnableVisibilityCulling : 1 = false;
    UPROPERTY()
    uint32 bMinDistanceEnabled : 1 = false;
    UPROPERTY()
    uint32 bMaxDistanceEnabled : 1 = false;
    UPROPERTY()
    uint32 bResetAgeOnAwaken : 1 = false;
    UPROPERTY()
    float MinDistance = 0.0f;
    UPROPERTY()
    ENiagaraExecutionStateManagement MinDistanceReaction = ENiagaraExecutionStateManagement::Awaken;
    UPROPERTY()
    float MaxDistance = 5000.0f;
    UPROPERTY()
    ENiagaraExecutionStateManagement MaxDistanceReaction = ENiagaraExecutionStateManagement::SleepAndLetParticlesFinish;
    UPROPERTY()
    ENiagaraExecutionStateManagement VisibilityCullReaction = ENiagaraExecutionStateManagement::SleepAndLetParticlesFinish;
    UPROPERTY()
    float VisibilityCullDelay = 1.0f;
};

}
