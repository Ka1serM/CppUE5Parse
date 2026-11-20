#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "NetworkPhysicsSettingsComponent.generated.h"

namespace ue5parse::runtime {

UCLASS()
class UNetworkPhysicsSettingsComponent : public UActorComponent {
    GENERATED_BODY()
public:
    explicit UNetworkPhysicsSettingsComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
