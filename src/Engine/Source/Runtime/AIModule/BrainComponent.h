#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "BrainComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UBrainComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UBrainComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
