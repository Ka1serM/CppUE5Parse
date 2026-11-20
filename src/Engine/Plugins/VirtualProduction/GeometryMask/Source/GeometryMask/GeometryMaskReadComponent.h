#pragma once

#include "Engine/Plugins/VirtualProduction/GeometryMask/Source/GeometryMask/GeometryMaskTypes.h"

#include "GeometryMaskReadComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UGeometryMaskReadComponent : public UGeometryMaskCanvasReferenceComponentBase {
    GENERATED_BODY()
public:
    explicit UGeometryMaskReadComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
