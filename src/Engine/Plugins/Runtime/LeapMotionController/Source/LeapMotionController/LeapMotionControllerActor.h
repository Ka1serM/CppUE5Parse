#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "LeapMotionControllerActor.generated.h"

namespace ue5parse::runtime {

class ULeapMotionControllerComponent;
class ULeapMotionImageComponent;

UCLASS()
class ALeapMotionControllerActor : public AActor {
    GENERATED_BODY()
public:
    explicit ALeapMotionControllerActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<ULeapMotionControllerComponent> LeapMotionControllerComponent = nullptr;
    UPROPERTY()
    TObjectPtr<ULeapMotionImageComponent> LeapMotionImageComponent = nullptr;
};

}
