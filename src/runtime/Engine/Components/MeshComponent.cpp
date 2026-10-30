#include "ue5parse/runtime/Engine/Components/MeshComponent.h"

// UE's Components/MeshComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

UMaterialInterface* UMeshComponent::GetMaterial(int32 ElementIndex) const {
    return OverrideMaterials.IsValidIndex(ElementIndex) ? OverrideMaterials[ElementIndex] : nullptr;
}

}
