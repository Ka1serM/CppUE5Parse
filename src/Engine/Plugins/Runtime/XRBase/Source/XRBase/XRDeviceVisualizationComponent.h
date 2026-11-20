#pragma once

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

#include "XRDeviceVisualizationComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UXRDeviceVisualizationComponent : public UStaticMeshComponent {
    GENERATED_BODY()
public:
    explicit UXRDeviceVisualizationComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
