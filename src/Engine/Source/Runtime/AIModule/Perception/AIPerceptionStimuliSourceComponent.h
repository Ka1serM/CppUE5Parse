#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "AIPerceptionStimuliSourceComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UAIPerceptionStimuliSourceComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UAIPerceptionStimuliSourceComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
