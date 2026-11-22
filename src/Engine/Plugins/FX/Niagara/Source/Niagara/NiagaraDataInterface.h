#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraDataInterfaceBase.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraDataInterface.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNiagaraDataInterface : public UNiagaraDataInterfaceBase {
    GENERATED_BODY()
public:
    explicit UNiagaraDataInterface(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

};

}
