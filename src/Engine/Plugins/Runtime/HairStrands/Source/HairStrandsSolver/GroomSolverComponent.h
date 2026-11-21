#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "GroomSolverComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UGroomSolverComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UGroomSolverComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
