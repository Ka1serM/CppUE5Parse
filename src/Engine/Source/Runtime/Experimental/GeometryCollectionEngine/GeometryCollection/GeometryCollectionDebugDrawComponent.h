#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "GeometryCollectionDebugDrawComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UGeometryCollectionDebugDrawComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UGeometryCollectionDebugDrawComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
