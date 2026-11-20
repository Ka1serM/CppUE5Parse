#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "ImageTrackerComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UImageTrackerComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UImageTrackerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
