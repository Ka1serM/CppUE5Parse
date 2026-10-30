#pragma once

#include "ue5parse/runtime/Engine/Components/MeshComponent.h"

#include "SkinnedMeshComponent.generated.h"

namespace ue5parse::runtime {

class USkinnedAsset;

// USkinnedMeshComponent: a placed skinned mesh (UE's Components/SkinnedMeshComponent.h).
UCLASS(Abstract)
class USkinnedMeshComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    USkinnedAsset* SkinnedAsset = nullptr;

    USkinnedAsset* GetSkinnedAsset() const { return SkinnedAsset; }
    int32 GetNumMaterials() const override;
    UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
};

}
