#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraCompileHash.h"

namespace ue5parse::runtime {

FArchive& operator<<(FArchive& Ar, FNiagaraCompileHash& Id) {
    return Ar << Id.DataHash;
}

}
