#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "VolumetricCloudComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

UCLASS()
class UVolumetricCloudComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UMaterialInterface> MaterialLifetimePtr = nullptr;

    explicit UVolumetricCloudComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
