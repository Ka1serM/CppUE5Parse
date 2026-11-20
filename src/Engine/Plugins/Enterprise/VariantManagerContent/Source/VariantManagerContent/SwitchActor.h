#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "SwitchActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ASwitchActor : public AActor {
    GENERATED_BODY()
public:
    explicit ASwitchActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<USceneComponent> SceneComponent = nullptr;
};

}
