#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "LeapMotionControllerComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULeapMotionControllerComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit ULeapMotionControllerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
