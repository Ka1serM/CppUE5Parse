#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "SubDSurfaceComponent.generated.h"

namespace ue5parse::runtime {

class UStaticMeshComponent;

UCLASS()
class USubDSurfaceComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit USubDSurfaceComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> DisplayMeshComponent = nullptr;
};

}
