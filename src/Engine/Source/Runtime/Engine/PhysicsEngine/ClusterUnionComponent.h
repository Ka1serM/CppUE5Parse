#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "ClusterUnionComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UClusterUnionComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UClusterUnionComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
