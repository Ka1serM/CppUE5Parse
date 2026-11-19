#pragma once

#include "Engine/Source/Runtime/Engine/Components/SphereComponent.h"

#include "DrawSphereComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UDrawSphereComponent : public USphereComponent {
    GENERATED_BODY()
public:
    explicit UDrawSphereComponent(const FObjectInitializer& ObjectInitializer);
};

}
