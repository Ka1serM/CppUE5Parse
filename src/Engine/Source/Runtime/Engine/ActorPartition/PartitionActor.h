#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "PartitionActor.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class APartitionActor : public AActor {
    GENERATED_BODY()
public:
    explicit APartitionActor(const FObjectInitializer& ObjectInitializer);
};

}
