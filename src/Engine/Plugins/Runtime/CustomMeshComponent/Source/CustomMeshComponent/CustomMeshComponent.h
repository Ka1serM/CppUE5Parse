#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "CustomMeshComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCustomMeshComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UCustomMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
