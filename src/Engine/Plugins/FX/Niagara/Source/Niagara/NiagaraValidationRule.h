#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraValidationRule.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNiagaraValidationRule : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraValidationRule(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    bool bIsConfigDisabled = false;
};

}
