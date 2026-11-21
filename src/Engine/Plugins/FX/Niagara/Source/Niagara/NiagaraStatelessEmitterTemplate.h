#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraStatelessEmitterTemplate.generated.h"

namespace ue5parse::runtime {

class UClass;

UCLASS()
class UNiagaraStatelessEmitterTemplate : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraStatelessEmitterTemplate(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TArray<TObjectPtr<UClass>> Modules;
    UPROPERTY()
    int32 NumShaderOutputVariables = 0;
};

}
