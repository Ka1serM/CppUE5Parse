#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "LineSetComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

UCLASS()
class ULineSetComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit ULineSetComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UMaterialInterface> LineMaterial = nullptr;
    UPROPERTY()
    FBoxSphereBounds Bounds;
    UPROPERTY()
    bool bBoundsDirty = true;
};

}
