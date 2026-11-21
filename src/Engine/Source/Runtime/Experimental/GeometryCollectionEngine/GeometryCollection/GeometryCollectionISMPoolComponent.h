#pragma once

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

#include "GeometryCollectionISMPoolComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UGeometryCollectionISMPoolComponent : public USceneComponent {
    GENERATED_BODY()
public:
    explicit UGeometryCollectionISMPoolComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
