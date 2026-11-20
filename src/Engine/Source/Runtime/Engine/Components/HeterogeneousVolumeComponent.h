#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "HeterogeneousVolumeComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UHeterogeneousVolumeComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit UHeterogeneousVolumeComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
