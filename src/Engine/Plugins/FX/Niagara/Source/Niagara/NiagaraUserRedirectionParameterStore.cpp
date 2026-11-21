#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraUserRedirectionParameterStore.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

namespace ue5parse::runtime {

bool FNiagaraUserRedirectionParameterStore::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("StructProperty") || Tag.TypeName.GetParameterName(0) != FName("NiagaraParameterStore"))
        return false;
    FNiagaraParameterStore::StaticStruct()->SerializeItem(Ar, static_cast<FNiagaraParameterStore*>(this));
    return true;
}

}
