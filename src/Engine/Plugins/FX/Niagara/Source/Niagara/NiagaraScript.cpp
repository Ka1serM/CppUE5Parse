#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraScript.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

namespace ue5parse::runtime {

bool FNiagaraVMExecutableByteCode::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("ArrayProperty"))
        return false;
    Ar << Data;
    return true;
}

}
