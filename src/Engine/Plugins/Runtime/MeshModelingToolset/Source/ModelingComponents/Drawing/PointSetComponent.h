#pragma once

#include "Engine/Source/Runtime/Core/Math/Box.h"
#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "PointSetComponent.generated.h"

namespace ue5parse::runtime {

class UMaterialInterface;

UCLASS()
class UPointSetComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UPointSetComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UMaterialInterface> PointMaterial = nullptr;
    UPROPERTY()
    FBoxSphereBounds Bounds;
    UPROPERTY()
    bool bBoundsDirty = true;
};

}
