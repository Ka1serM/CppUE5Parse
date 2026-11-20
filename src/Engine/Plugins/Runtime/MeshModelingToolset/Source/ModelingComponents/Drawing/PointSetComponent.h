#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "PointSetComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPointSetComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UPointSetComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
