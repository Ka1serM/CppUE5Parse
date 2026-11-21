#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraDataChannelLayoutInfo.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraDataChannelVariable.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraDataSetCompiledData.h"
#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/Engine/EngineBaseTypes.h"

#include "NiagaraDataChannel.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNiagaraDataChannel : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraDataChannel(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FName ChannelName;
    UPROPERTY()
    TArray<FNiagaraVariable> Variables;
    UPROPERTY()
    bool bKeepPreviousFrameData = true;
    UPROPERTY()
    FNiagaraDataSetCompiledData CompiledData;
    UPROPERTY()
    FNiagaraDataSetCompiledData CompiledDataGPU;
    UPROPERTY()
    FNiagaraDataChannelGameDataLayout GameDataLayout;
    UPROPERTY()
    TArray<FNiagaraDataChannelVariable> ChannelVariables;
    UPROPERTY()
    bool bEnforceTickGroupReadWriteOrder = false;
    UPROPERTY()
    TEnumAsByte<ETickingGroup> FinalWriteTickGroup = ETickingGroup::TG_EndPhysics;
};

}
