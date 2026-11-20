#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "GeometryMaskTypes.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UGeometryMaskCanvasReferenceComponentBase : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UGeometryMaskCanvasReferenceComponentBase(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
