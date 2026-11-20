#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "MotionControllerComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMotionControllerComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UMotionControllerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
