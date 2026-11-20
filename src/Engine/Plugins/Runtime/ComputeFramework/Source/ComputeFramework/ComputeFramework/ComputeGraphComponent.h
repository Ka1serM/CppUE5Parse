#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "ComputeGraphComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UComputeGraphComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UComputeGraphComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
