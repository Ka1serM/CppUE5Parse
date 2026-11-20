#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"
#include "Engine/Source/Runtime/Engine/Engine/Scene.h"

#include "PostProcessComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPostProcessComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FPostProcessSettings Settings;
    UPROPERTY()
    FGuid VolumeGuid;

    explicit UPostProcessComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
