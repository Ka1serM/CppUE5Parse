#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "DataflowComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UDataflowComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UDataflowComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
