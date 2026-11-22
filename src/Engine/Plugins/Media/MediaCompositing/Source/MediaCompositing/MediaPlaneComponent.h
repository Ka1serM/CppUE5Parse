#pragma once

#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "MediaPlaneComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UMediaPlaneComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    explicit UMediaPlaneComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
