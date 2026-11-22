#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "LocalFogVolumeComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULocalFogVolumeComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit ULocalFogVolumeComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
