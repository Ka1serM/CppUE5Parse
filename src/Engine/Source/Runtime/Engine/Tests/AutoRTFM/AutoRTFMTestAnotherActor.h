#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "AutoRTFMTestAnotherActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AAutoRTFMTestAnotherActor : public AActor {
    GENERATED_BODY()
public:
    explicit AAutoRTFMTestAnotherActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
