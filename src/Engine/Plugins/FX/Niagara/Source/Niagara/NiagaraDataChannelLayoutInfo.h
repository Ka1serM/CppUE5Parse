#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"
#include "Engine/Source/Runtime/Core/Containers/Map.h"

#include "NiagaraDataChannelLayoutInfo.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraDataChannelGameDataLayout {
    GENERATED_BODY()

    UPROPERTY()
    TMap<FNiagaraVariableBase, int32> VariableIndices;
    UPROPERTY()
    TArray<FNiagaraLwcStructConverter> LwcConverters;
};

}
