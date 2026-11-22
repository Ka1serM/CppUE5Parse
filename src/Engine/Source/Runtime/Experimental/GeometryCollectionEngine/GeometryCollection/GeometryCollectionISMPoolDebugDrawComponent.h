#pragma once

#include "Engine/Source/Runtime/Engine/Debug/DebugDrawComponent.h"

#include "GeometryCollectionISMPoolDebugDrawComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UGeometryCollectionISMPoolDebugDrawComponent : public UDebugDrawComponent {
    GENERATED_BODY()
public:
    explicit UGeometryCollectionISMPoolDebugDrawComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
