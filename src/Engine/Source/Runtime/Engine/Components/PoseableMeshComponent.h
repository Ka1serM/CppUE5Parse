#pragma once

#include "Engine/Source/Runtime/Engine/Components/SkinnedMeshComponent.h"

#include "PoseableMeshComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPoseableMeshComponent : public USkinnedMeshComponent {
    GENERATED_BODY()
public:
    explicit UPoseableMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
