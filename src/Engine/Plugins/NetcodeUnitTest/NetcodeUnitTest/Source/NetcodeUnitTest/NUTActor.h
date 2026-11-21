#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "NUTActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ANUTActor : public AActor {
    GENERATED_BODY()
public:
    explicit ANUTActor(const FObjectInitializer& ObjectInitializer);
};

}
