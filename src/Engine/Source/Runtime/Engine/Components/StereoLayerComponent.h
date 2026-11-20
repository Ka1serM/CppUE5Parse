#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "StereoLayerComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UStereoLayerComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UStereoLayerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
