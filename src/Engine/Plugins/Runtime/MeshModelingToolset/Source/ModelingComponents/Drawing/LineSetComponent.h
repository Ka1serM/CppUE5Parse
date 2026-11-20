#pragma once

#include "Engine/Source/Runtime/Engine/Components/MeshComponent.h"

#include "LineSetComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class ULineSetComponent : public UMeshComponent {
    GENERATED_BODY()
public:
    explicit ULineSetComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
