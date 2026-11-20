#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/RigidBodyBase.h"

#include "PhysicsThruster.generated.h"

namespace ue5parse::runtime {

class UPhysicsThrusterComponent;

UCLASS()
class APhysicsThruster : public ARigidBodyBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPhysicsThrusterComponent> ThrusterComponent = nullptr;

    explicit APhysicsThruster(const FObjectInitializer& ObjectInitializer);
};

}
