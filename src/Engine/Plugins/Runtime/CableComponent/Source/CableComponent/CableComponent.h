#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "CableComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UCableComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UCableComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
