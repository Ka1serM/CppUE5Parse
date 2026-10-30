#pragma once

#include "ue5parse/runtime/Engine/Components/PrimitiveComponent.h"

#include "MeshComponent.generated.h"

namespace ue5parse::runtime {

// UMeshComponent: a primitive with material slots that override its mesh's (UE's Components/MeshComponent.h).
UCLASS(Abstract)
class UMeshComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TArray<UMaterialInterface*> OverrideMaterials;

    UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
};

}
