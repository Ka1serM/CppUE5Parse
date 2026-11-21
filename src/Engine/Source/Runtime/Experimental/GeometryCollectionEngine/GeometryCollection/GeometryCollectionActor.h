#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "GeometryCollectionActor.generated.h"

namespace ue5parse::runtime {

class UGeometryCollectionComponent;
class UGeometryCollectionDebugDrawComponent;

UCLASS()
class AGeometryCollectionActor : public AActor {
    GENERATED_BODY()
public:
    explicit AGeometryCollectionActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UGeometryCollectionComponent> GeometryCollectionComponent = nullptr;
    UPROPERTY()
    TObjectPtr<UGeometryCollectionDebugDrawComponent> GeometryCollectionDebugDrawComponent = nullptr;
};

}
