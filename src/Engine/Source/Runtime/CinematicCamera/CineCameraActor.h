#pragma once

#include "Engine/Source/Runtime/Engine/Camera/CameraActor.h"

#include "CineCameraActor.generated.h"

namespace ue5parse::runtime {

class UCineCameraComponent;

UCLASS()
class ACineCameraActor : public ACameraActor {
    GENERATED_BODY()
public:
    explicit ACineCameraActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UCineCameraComponent> CineCameraComponent = nullptr;
};

}
