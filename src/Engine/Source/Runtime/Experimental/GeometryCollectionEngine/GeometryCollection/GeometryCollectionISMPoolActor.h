#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "GeometryCollectionISMPoolActor.generated.h"

namespace ue5parse::runtime {

class UGeometryCollectionISMPoolComponent;
class UGeometryCollectionISMPoolDebugDrawComponent;

UCLASS()
class AGeometryCollectionISMPoolActor : public AActor {
    GENERATED_BODY()
public:
    explicit AGeometryCollectionISMPoolActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UGeometryCollectionISMPoolComponent> ISMPoolComp = nullptr;
    UPROPERTY()
    TObjectPtr<UGeometryCollectionISMPoolDebugDrawComponent> ISMPoolDebugDrawComp = nullptr;
};

}
