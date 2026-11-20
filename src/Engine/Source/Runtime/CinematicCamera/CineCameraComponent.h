#pragma once

#include "Engine/Source/Runtime/Engine/Camera/CameraComponent.h"

#include "CineCameraComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCineCameraComponent : public UCameraComponent {
    GENERATED_BODY()
public:
    explicit UCineCameraComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
