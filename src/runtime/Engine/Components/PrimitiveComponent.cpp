#include "ue5parse/runtime/Engine/Components/PrimitiveComponent.h"

#include "ue5parse/runtime/Engine/Materials/MaterialInstanceDynamic.h"

// UE's Components/PrimitiveComponent.cpp (UE 5.3).

namespace ue5parse::runtime {

UMaterialInstanceDynamic* UPrimitiveComponent::CreateDynamicMaterialInstance(int32 ElementIndex, UMaterialInterface* SourceMaterial, FName OptionalName) {
    if (SourceMaterial != nullptr) SetMaterial(ElementIndex, SourceMaterial);
    UMaterialInterface* MaterialInstance = GetMaterial(ElementIndex);
    UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(MaterialInstance);
    if (MaterialInstance != nullptr && MID == nullptr) {
        MID = UMaterialInstanceDynamic::Create(MaterialInstance, this, OptionalName);
        SetMaterial(ElementIndex, MID);
    }
    return MID;
}

}
