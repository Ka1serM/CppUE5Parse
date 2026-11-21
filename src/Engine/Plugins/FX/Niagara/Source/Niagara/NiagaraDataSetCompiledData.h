#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCommon.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"

#include "NiagaraDataSetCompiledData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraVariableLayoutInfo {
    GENERATED_BODY()

    UPROPERTY()
    uint16 FloatComponentStart = 0;
    UPROPERTY()
    uint16 Int32ComponentStart = 0;
    UPROPERTY()
    FNiagaraTypeLayoutInfo LayoutInfo;
    UPROPERTY()
    uint16 HalfComponentStart = 0;
};

USTRUCT()
struct FNiagaraDataSetCompiledData {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FNiagaraVariableBase> Variables;
    UPROPERTY()
    TArray<FNiagaraVariableLayoutInfo> VariableLayouts;
    UPROPERTY()
    uint32 TotalFloatComponents = 0;
    UPROPERTY()
    uint32 TotalInt32Components = 0;
    UPROPERTY()
    uint32 bNeedsPersistentIDs : 1 = 0;
    UPROPERTY()
    FNiagaraDataSetID ID;
    UPROPERTY()
    ENiagaraSimTarget SimTarget = ENiagaraSimTarget::CPUSim;
    UPROPERTY()
    uint32 bRequiresPersistentIDs : 1 = 0;
    UPROPERTY()
    uint32 TotalHalfComponents = 0;
};

}
