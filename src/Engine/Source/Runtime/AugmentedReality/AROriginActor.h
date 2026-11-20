#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "AROriginActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AAROriginActor : public AActor {
    GENERATED_BODY()
public:
    explicit AAROriginActor(const FObjectInitializer& ObjectInitializer);
};

}
