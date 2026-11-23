#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraShared.h"

namespace ue5parse::runtime {

void FNiagaraShaderMapPointerTable::LoadFromArchive(FArchive& Ar) {
    FShaderMapPointerTable::LoadFromArchive(Ar);
    Ar << DITypes;
}

}
