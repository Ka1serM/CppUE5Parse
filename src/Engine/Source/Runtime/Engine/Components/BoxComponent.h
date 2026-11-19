#pragma once

#include "Engine/Source/Runtime/Engine/Components/ShapeComponent.h"

#include "BoxComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBoxComponent : public UShapeComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FVector BoxExtent = FVector(32.0f, 32.0f, 32.0f);

    explicit UBoxComponent(const FObjectInitializer& ObjectInitializer);
};

}
