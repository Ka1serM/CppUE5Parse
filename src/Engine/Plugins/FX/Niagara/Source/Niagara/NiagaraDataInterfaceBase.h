#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraNotifyOnChanged.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraDataInterfaceBase.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNiagaraDataInterfaceBase : public UNiagaraNotifyOnChanged {
    GENERATED_BODY()
public:
    explicit UNiagaraDataInterfaceBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

};

}
