#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "PaperTerrainComponent.generated.h"

namespace ue5parse::runtime {

class UPaperTerrainSplineComponent;

UCLASS()
class UPaperTerrainComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPaperTerrainSplineComponent> AssociatedSpline = nullptr;

    explicit UPaperTerrainComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
