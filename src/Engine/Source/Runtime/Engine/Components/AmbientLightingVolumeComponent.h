#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "AmbientLightingVolumeComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAmbientLightingVolumeComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UAmbientLightingVolumeComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
