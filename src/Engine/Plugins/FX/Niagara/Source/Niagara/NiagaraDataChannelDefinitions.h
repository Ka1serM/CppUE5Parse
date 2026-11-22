#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "NiagaraDataChannelDefinitions.generated.h"

namespace ue5parse::runtime {

class UNiagaraDataChannel;

UCLASS()
class UNiagaraDataChannelDefinitions : public UObject {
    GENERATED_BODY()
public:
    explicit UNiagaraDataChannelDefinitions(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TArray<TObjectPtr<UNiagaraDataChannel>> DataChannels;
};

}
