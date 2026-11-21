#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraNotifyOnChanged.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraMergeable.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNiagaraMergeable : public UNiagaraNotifyOnChanged {
    GENERATED_BODY()
public:
    explicit UNiagaraMergeable(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

};

}
