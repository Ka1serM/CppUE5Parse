#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "DataflowActor.generated.h"

namespace ue5parse::runtime {

class UDataflowComponent;

UCLASS()
class ADataflowActor : public AActor {
    GENERATED_BODY()
public:
    explicit ADataflowActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UDataflowComponent> DataflowComponent = nullptr;
};

}
