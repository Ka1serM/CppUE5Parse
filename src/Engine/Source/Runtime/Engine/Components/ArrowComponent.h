#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "ArrowComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UArrowComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UArrowComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
