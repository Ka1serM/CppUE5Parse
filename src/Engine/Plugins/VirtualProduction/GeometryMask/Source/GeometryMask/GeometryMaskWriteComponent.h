#pragma once

#include "Engine/Plugins/VirtualProduction/GeometryMask/Source/GeometryMask/GeometryMaskTypes.h"

#include "GeometryMaskWriteComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UGeometryMaskWriteMeshComponent : public UGeometryMaskCanvasReferenceComponentBase {
    GENERATED_BODY()
public:
    explicit UGeometryMaskWriteMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
