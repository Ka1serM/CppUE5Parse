#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraParameterCollection.h"

namespace ue5parse::runtime {

UNiagaraParameterCollection::UNiagaraParameterCollection(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Namespace = GetFName();
    DefaultInstance = ObjectInitializer.CreateDefaultSubobject<UNiagaraParameterCollectionInstance>(FName("Default Instance"));
}

}
