#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "VectorFieldComponent.generated.h"

namespace ue5parse::runtime {

class UVectorField;

UCLASS()
class UVectorFieldComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UVectorField> VectorField = nullptr;

    explicit UVectorFieldComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
