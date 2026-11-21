#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "DebugDrawComponent.generated.h"

namespace ue5parse::runtime {

UCLASS(abstract)
class UDebugDrawComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UDebugDrawComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
