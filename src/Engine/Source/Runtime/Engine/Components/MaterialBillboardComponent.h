#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "MaterialBillboardComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMaterialBillboardComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UMaterialBillboardComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
