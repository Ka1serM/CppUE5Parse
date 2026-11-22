#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "GeometryCacheComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UGeometryCacheComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UGeometryCacheComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
