#pragma once

#include "Engine/Source/Runtime/Engine/Components/SkinnedMeshComponent.h"

#include "DestructibleComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UDestructibleComponent : public USkinnedMeshComponent {
    GENERATED_BODY()
public:
    explicit UDestructibleComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
