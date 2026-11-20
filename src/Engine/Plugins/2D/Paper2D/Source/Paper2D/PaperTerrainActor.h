#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "PaperTerrainActor.generated.h"

namespace ue5parse::runtime {

class UPaperTerrainComponent;
class UPaperTerrainSplineComponent;

UCLASS()
class APaperTerrainActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<USceneComponent> DummyRoot = nullptr;
    UPROPERTY()
    TObjectPtr<UPaperTerrainSplineComponent> SplineComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UPaperTerrainComponent> RenderComponent = nullptr;

    explicit APaperTerrainActor(const FObjectInitializer& ObjectInitializer);
};

}
