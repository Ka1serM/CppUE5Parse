#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "ProceduralMeshComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UProceduralMeshComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UProceduralMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
