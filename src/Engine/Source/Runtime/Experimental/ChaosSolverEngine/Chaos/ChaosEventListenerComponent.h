#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "ChaosEventListenerComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UChaosEventListenerComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UChaosEventListenerComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
