#pragma once

#include "Engine/Source/Runtime/Engine/Components/SplineComponent.h"

#include "PaperTerrainSplineComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPaperTerrainSplineComponent : public USplineComponent {
    GENERATED_BODY()
public:
    explicit UPaperTerrainSplineComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
