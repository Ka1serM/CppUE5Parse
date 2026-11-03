#include "ue5parse/runtime/Engine/Components/MeshComponent.h"

#include "ue5parse/runtime/Engine/Materials/MaterialInstanceDynamic.h"

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

// UE caches the slots of each parameter name (CacheMaterialParameterNameIndices); the slots are few, so they are looked
// up on each call.
void UMeshComponent::SetScalarParameterValueOnMaterials(const FName ParameterName, const float ParameterValue) {
    const FMaterialParameterInfo ParameterInfo(ParameterName);
    for (int32 MaterialIndex = 0; MaterialIndex < GetNumMaterials(); ++MaterialIndex) {
        const UMaterialInterface* Material = GetMaterial(MaterialIndex);
        float Current = 0;
        if (Material == nullptr || !Material->GetScalarParameterValue(ParameterInfo, Current)) continue;
        if (UMaterialInstanceDynamic* MID = CreateDynamicMaterialInstance(MaterialIndex)) MID->SetScalarParameterValue(ParameterName, ParameterValue);
    }
}

// UE reads the defaults when it caches the slots of each parameter name (CacheMaterialParameterNameIndices); a later slot
// replaces an earlier slot's default.
float UMeshComponent::GetScalarParameterDefaultValue(const FName ParameterName) const {
    const FMaterialParameterInfo ParameterInfo(ParameterName);
    float DefaultValue = 0;
    for (int32 MaterialIndex = 0; MaterialIndex < GetNumMaterials(); ++MaterialIndex) {
        const UMaterialInterface* Material = GetMaterial(MaterialIndex);
        if (Material != nullptr) Material->GetScalarParameterDefaultValue(ParameterInfo, DefaultValue);
    }
    return DefaultValue;
}

void UMeshComponent::SetVectorParameterValueOnMaterials(const FName ParameterName, const FVector ParameterValue) {
    const FMaterialParameterInfo ParameterInfo(ParameterName);
    for (int32 MaterialIndex = 0; MaterialIndex < GetNumMaterials(); ++MaterialIndex) {
        const UMaterialInterface* Material = GetMaterial(MaterialIndex);
        FLinearColor Current;
        if (Material == nullptr || !Material->GetVectorParameterValue(ParameterInfo, Current)) continue;
        if (UMaterialInstanceDynamic* MID = CreateDynamicMaterialInstance(MaterialIndex)) MID->SetVectorParameterValue(ParameterName, FLinearColor(ParameterValue));
    }
}

}
