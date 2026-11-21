#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraTypes.h"

#include "NiagaraDataChannelVariable.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FNiagaraDataChannelVariable : public FNiagaraVariableBase {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) { return FNiagaraVariableBase::Serialize(Ar); }
};

template <>
struct TStructOpsTypeTraits<FNiagaraDataChannelVariable> : TStructOpsTypeTraitsBase2<FNiagaraDataChannelVariable> {
    enum { WithSerializer = true };
};

}
