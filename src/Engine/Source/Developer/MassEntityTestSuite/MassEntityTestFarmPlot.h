#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "MassEntityTestFarmPlot.generated.h"

namespace ue5parse::runtime {

class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class AMassEntityTestFarmPlot : public AActor {
    GENERATED_BODY()
public:
    explicit AMassEntityTestFarmPlot(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HarvestIconISMC = nullptr;
};

}
