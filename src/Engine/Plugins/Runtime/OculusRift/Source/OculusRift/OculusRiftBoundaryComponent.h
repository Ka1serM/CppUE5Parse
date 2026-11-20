#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "OculusRiftBoundaryComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UOculusRiftBoundaryComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UOculusRiftBoundaryComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
