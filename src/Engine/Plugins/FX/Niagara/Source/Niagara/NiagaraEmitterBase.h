#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraEmitterBase.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNiagaraEmitterBase : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraEmitterBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FString UniqueEmitterName;
};

}
