#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "MaterialInstanceActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AMaterialInstanceActor : public AActor {
    GENERATED_BODY()
public:
    explicit AMaterialInstanceActor(const FObjectInitializer& ObjectInitializer);
};

}
