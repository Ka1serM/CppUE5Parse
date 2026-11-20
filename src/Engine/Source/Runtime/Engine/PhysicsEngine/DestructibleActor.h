#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "DestructibleActor.generated.h"

namespace ue5parse::runtime {

class UDestructibleComponent;

UCLASS()
class ADestructibleActor : public AActor {
    GENERATED_BODY()
public:
    explicit ADestructibleActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UDestructibleComponent> DestructibleComponent = nullptr;
};

}
