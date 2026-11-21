#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "ChaosDebugDrawComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UChaosDebugDrawComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UChaosDebugDrawComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
