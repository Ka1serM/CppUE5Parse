#pragma once

#include "ue5parse/runtime/Engine/Components/SceneComponent.h"

#include "PrimitiveComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

// UPrimitiveComponent: a scene component with geometry that is rendered or collides (UE's
// Components/PrimitiveComponent.h).
UCLASS(Abstract)
class UPrimitiveComponent : public USceneComponent {
    GENERATED_BODY()
public:
    // The number of material slots; the base class has none.
    virtual int32 GetNumMaterials() const { return 0; }
    // The material of a slot: an override, else the mesh's.
    virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const { return nullptr; }
};

}
