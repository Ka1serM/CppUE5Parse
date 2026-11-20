#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "MRMeshComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMRMeshComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UMRMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
