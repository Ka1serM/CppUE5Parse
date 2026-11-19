#pragma once

#include "Engine/Source/Runtime/Engine/Components/ShapeComponent.h"

#include "SphereComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class USphereComponent : public UShapeComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    float SphereRadius = 32.0f;

    explicit USphereComponent(const FObjectInitializer& ObjectInitializer);
};

}
