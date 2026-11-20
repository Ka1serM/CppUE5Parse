#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "CameraShakeSourceComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCameraShakeSourceComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UCameraShakeSourceComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
