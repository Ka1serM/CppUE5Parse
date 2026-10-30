#include "ue5parse/runtime/Engine/Components/MeshComponent.h"

// UE's Components/MeshComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

UMaterialInterface* UMeshComponent::GetMaterial(int32 ElementIndex) const {
    return OverrideMaterials.IsValidIndex(ElementIndex) ? OverrideMaterials[ElementIndex] : nullptr;
}

void UMeshComponent::SetMaterial(int32 ElementIndex, UMaterialInterface* Material) {
    if (ElementIndex < 0) return;
    if (OverrideMaterials.IsValidIndex(ElementIndex) && OverrideMaterials[ElementIndex] == Material) return;
    if (OverrideMaterials.Num() <= ElementIndex) OverrideMaterials.AddZeroed(ElementIndex + 1 - OverrideMaterials.Num());
    OverrideMaterials[ElementIndex] = Material;
}

void UMeshComponent::SetMaterialByName(FName MaterialSlotName, UMaterialInterface* Material) {
    int32 MaterialIndex = GetMaterialIndex(MaterialSlotName);
    if (MaterialIndex < 0) return;
    SetMaterial(MaterialIndex, Material);
}

TArray<UMaterialInterface*> UMeshComponent::GetMaterials() const {
    TArray<UMaterialInterface*> OutMaterials;
    const int32 TotalNumMaterials = GetNumMaterials();
    if (TotalNumMaterials > 0) {
        OutMaterials.AddZeroed(TotalNumMaterials);
        for (int32 MaterialIndex = 0; MaterialIndex < TotalNumMaterials; ++MaterialIndex) OutMaterials[MaterialIndex] = GetMaterial(MaterialIndex);
    }
    return OutMaterials;
}

}
