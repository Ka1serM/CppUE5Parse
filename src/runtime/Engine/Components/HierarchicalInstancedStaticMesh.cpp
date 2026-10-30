#include "ue5parse/runtime/Engine/Components/HierarchicalInstancedStaticMeshComponent.h"

#include "ue5parse/runtime/Engine/StaticMeshResources.h"

// UE's Engine/HierarchicalInstancedStaticMesh.cpp (UE 5.3).

namespace ue5parse::runtime {

void UHierarchicalInstancedStaticMeshComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd()) return;
    SkipBulkSerializedArray(Ar);   // ClusterTree: nothing reads it.
}

}
