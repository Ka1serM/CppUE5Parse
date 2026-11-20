#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "MatineeActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AMatineeActor : public AActor {
    GENERATED_BODY()
public:
    explicit AMatineeActor(const FObjectInitializer& ObjectInitializer);
};

}
