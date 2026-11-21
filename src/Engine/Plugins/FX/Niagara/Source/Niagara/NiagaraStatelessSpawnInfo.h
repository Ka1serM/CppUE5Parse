#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraStatelessDistribution.h"

#include "NiagaraStatelessSpawnInfo.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ENiagaraStatelessSpawnInfoType : int32 { Burst, Rate };

USTRUCT()
struct FNiagaraStatelessSpawnInfo {
    GENERATED_BODY()

    UPROPERTY()
    ENiagaraStatelessSpawnInfoType Type = ENiagaraStatelessSpawnInfoType::Burst;
    UPROPERTY()
    float SpawnTime = 0.0f;
    UPROPERTY()
    FNiagaraDistributionRangeInt Amount;
    UPROPERTY()
    FNiagaraDistributionRangeFloat Rate;
    UPROPERTY()
    uint8 bSpawnProbabilityEnabled : 1 = false;
    UPROPERTY()
    FNiagaraDistributionRangeFloat SpawnProbability;
    UPROPERTY()
    uint8 bEnabled : 1 = true;
    UPROPERTY()
    uint8 bLoopCountLimitEnabled : 1 = false;
    UPROPERTY()
    FNiagaraDistributionRangeInt LoopCountLimit;
};

}
