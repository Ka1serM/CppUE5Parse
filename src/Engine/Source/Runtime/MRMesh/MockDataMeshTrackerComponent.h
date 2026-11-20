#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "MockDataMeshTrackerComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMockDataMeshTrackerComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UMockDataMeshTrackerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
