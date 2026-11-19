#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "Engine/Source/Runtime/Engine/Materials/MaterialInstanceDynamic.h"

namespace ue5parse::runtime {

UPrimitiveComponent::UPrimitiveComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    BodyInstance.CollisionProfileName = FName("BlockAll");
}

UMaterialInstanceDynamic* UPrimitiveComponent::CreateDynamicMaterialInstance(int32 ElementIndex, UMaterialInterface* SourceMaterial, FName OptionalName) {
    if (SourceMaterial != nullptr)
        SetMaterial(ElementIndex, SourceMaterial);
    UMaterialInterface* MaterialInstance = GetMaterial(ElementIndex);
    UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(MaterialInstance);
    if (MaterialInstance != nullptr && MID == nullptr) {
        MID = UMaterialInstanceDynamic::Create(MaterialInstance, this, OptionalName);
        SetMaterial(ElementIndex, MID);
    }
    return MID;
}

}
