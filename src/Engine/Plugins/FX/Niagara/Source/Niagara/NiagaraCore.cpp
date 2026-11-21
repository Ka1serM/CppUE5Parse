#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCore.h"

namespace ue5parse::runtime {

bool FNiagaraVariableCommonReference::Serialize(FArchive& Ar) {
    Ar << Name << UnderlyingType;
    return true;
}

}
