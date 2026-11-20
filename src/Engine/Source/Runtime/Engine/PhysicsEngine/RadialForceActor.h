#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/RigidBodyBase.h"

#include "RadialForceActor.generated.h"

namespace ue5parse::runtime {

class URadialForceComponent;

UCLASS()
class ARadialForceActor : public ARigidBodyBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<URadialForceComponent> ForceComponent = nullptr;

    explicit ARadialForceActor(const FObjectInitializer& ObjectInitializer);
};

}
