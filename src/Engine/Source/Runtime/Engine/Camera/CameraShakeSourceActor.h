#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "CameraShakeSourceActor.generated.h"

namespace ue5parse::runtime {

class UCameraShakeSourceComponent;

UCLASS()
class ACameraShakeSourceActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UCameraShakeSourceComponent> CameraShakeSourceComponent = nullptr;

    explicit ACameraShakeSourceActor(const FObjectInitializer& ObjectInitializer);
};

}
