#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "MeshTrackerComponent.generated.h"

namespace ue5parse::runtime {

class UBoxComponent;

UCLASS()
class UMeshTrackerComponent : public USceneComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UBoxComponent> BoundingVolume = nullptr;

    explicit UMeshTrackerComponent(const FObjectInitializer& ObjectInitializer);
};

}
