#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "GeometryCollectionRenderLevelSetActor.generated.h"

namespace ue5parse::runtime {

class UMaterial;
class UPostProcessComponent;

UCLASS()
class AGeometryCollectionRenderLevelSetActor : public AActor {
    GENERATED_BODY()
public:
    explicit AGeometryCollectionRenderLevelSetActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UMaterial> RayMarchMaterial = nullptr;
    UPROPERTY()
    TObjectPtr<UPostProcessComponent> PostProcessComponent = nullptr;
};

}
