#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraActor.h"

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraComponent.h"

namespace ue5parse::runtime {

ANiagaraActor::ANiagaraActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    NiagaraComponent = ObjectInitializer.CreateDefaultSubobject<UNiagaraComponent>(FName("NiagaraComponent0"));
    RootComponent = NiagaraComponent;
}

}
