#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "OculusBoundaryComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UOculusBoundaryComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UOculusBoundaryComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
