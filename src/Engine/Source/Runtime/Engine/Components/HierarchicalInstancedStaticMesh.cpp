#include "Engine/Source/Runtime/Engine/Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

namespace ue5parse::runtime {

void UHierarchicalInstancedStaticMeshComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.AtEnd())
        return;
    SkipBulkSerializedArray(Ar);
}

}
