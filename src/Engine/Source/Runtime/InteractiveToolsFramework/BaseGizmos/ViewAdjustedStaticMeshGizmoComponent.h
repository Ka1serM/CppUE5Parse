#pragma once

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

#include "ViewAdjustedStaticMeshGizmoComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UViewAdjustedStaticMeshGizmoComponent : public UStaticMeshComponent {
    GENERATED_BODY()
public:
    explicit UViewAdjustedStaticMeshGizmoComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
