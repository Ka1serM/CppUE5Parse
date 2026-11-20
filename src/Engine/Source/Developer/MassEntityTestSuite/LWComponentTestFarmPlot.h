#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "LWComponentTestFarmPlot.generated.h"

namespace ue5parse::runtime {

class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class ALWFragmentTestFarmPlot : public AActor {
    GENERATED_BODY()
public:
    explicit ALWFragmentTestFarmPlot(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UHierarchicalInstancedStaticMeshComponent> HarvestIconISMC = nullptr;
};

}
