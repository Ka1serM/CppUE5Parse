#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "WidgetComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UWidgetComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UWidgetComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
