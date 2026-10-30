#include "ue5parse/runtime/Engine/Components/SkinnedMeshComponent.h"

#include "ue5parse/runtime/Engine/Engine/SkinnedAsset.h"
#include "ue5parse/runtime/Engine/Engine/SkinnedAssetCommon.h"

// UE's Components/SkinnedMeshComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

int32 USkinnedMeshComponent::GetNumMaterials() const { return SkinnedAsset ? SkinnedAsset->GetMaterials().Num() : 0; }

UMaterialInterface* USkinnedMeshComponent::GetMaterial(int32 MaterialIndex) const {
    if (UMaterialInterface* Override = Super::GetMaterial(MaterialIndex)) return Override;
    if (SkinnedAsset == nullptr || !SkinnedAsset->GetMaterials().IsValidIndex(MaterialIndex)) return nullptr;
    return SkinnedAsset->GetMaterials()[MaterialIndex].MaterialInterface;
}

}
