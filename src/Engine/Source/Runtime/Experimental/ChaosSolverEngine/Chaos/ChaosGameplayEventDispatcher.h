#pragma once

#include "Engine/Source/Runtime/Experimental/ChaosSolverEngine/Chaos/ChaosEventListenerComponent.h"

#include "ChaosGameplayEventDispatcher.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UChaosGameplayEventDispatcher : public UChaosEventListenerComponent {
    GENERATED_BODY()
public:
    explicit UChaosGameplayEventDispatcher(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
