#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraNotifyOnChanged.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNiagaraNotifyOnChanged : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraNotifyOnChanged(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

};

}
