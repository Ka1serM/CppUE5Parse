#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "LeapMotionHandActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ALeapMotionHandActor : public AActor {
    GENERATED_BODY()
public:
    explicit ALeapMotionHandActor(const FObjectInitializer& ObjectInitializer);
};

}
