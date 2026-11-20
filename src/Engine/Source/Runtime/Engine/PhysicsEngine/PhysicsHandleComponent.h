#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "PhysicsHandleComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UPhysicsHandleComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UPhysicsHandleComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
