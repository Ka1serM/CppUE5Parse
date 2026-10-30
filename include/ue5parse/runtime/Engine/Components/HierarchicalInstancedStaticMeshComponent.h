#pragma once

#include "ue5parse/runtime/Engine/Components/InstancedStaticMeshComponent.h"

#include "HierarchicalInstancedStaticMeshComponent.generated.h"

namespace ue5parse::runtime {

// UHierarchicalInstancedStaticMeshComponent: instances in a cluster tree for culling (UE's
// Components/HierarchicalInstancedStaticMeshComponent.h). The tree is not kept.
UCLASS()
class UHierarchicalInstancedStaticMeshComponent : public UInstancedStaticMeshComponent {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;
};

}
