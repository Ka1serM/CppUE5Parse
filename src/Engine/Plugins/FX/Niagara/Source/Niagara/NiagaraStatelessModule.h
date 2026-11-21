#pragma once

#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraMergeable.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraStatelessModule.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNiagaraStatelessModule : public UNiagaraMergeable {
    GENERATED_BODY()
public:
    explicit UNiagaraStatelessModule(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    uint32 bModuleEnabled : 1 = true;
};

}
