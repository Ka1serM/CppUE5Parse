#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "GeometryCollectionDebugDrawActor.generated.h"

namespace ue5parse::runtime {

class UBillboardComponent;

UCLASS()
class AGeometryCollectionDebugDrawActor : public AActor {
    GENERATED_BODY()
public:
    explicit AGeometryCollectionDebugDrawActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UBillboardComponent> SpriteComponent = nullptr;
};

}
