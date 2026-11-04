#pragma once

#include "Engine/Source/Runtime/Engine/Components/InstancedStaticMeshComponent.h"

#include "HierarchicalInstancedStaticMeshComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UHierarchicalInstancedStaticMeshComponent : public UInstancedStaticMeshComponent {
    GENERATED_BODY()
public:
    void Serialize(FArchive& Ar) override;
};

}
