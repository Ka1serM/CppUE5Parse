#pragma once

#include "Engine/Source/Runtime/AIModule/BrainComponent.h"

#include "StateTreeBrainComponent.generated.h"

namespace ue5parse::runtime {

class UStateTree;

UCLASS()
class UStateTreeBrainComponent : public UBrainComponent {
    GENERATED_BODY()
public:
    explicit UStateTreeBrainComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    TObjectPtr<UStateTree> StateTree = nullptr;
};

}
