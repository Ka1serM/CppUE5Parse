#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "ControlRigComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UControlRigComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UControlRigComponent(const FObjectInitializer& ObjectInitializer);
};

}
