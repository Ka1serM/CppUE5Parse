#pragma once

#include "Engine/Source/Runtime/Engine/Components/InstancedStaticMeshComponent.h"

#include "HLODInstancedStaticMeshComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UHLODInstancedStaticMeshComponent : public UInstancedStaticMeshComponent {
    GENERATED_BODY()
public:
    explicit UHLODInstancedStaticMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
