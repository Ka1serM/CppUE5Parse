#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "ExponentialHeightFogComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UExponentialHeightFogComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    float FogDensity = 0.02f;
    UPROPERTY()
    bool bEnableVolumetricFog = false;

    explicit UExponentialHeightFogComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
