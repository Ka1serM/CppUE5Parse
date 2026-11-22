#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "PawnNoiseEmitterComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPawnNoiseEmitterComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UPawnNoiseEmitterComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
