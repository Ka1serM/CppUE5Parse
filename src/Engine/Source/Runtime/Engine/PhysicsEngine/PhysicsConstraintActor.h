#pragma once

#include "Engine/Source/Runtime/Engine/PhysicsEngine/RigidBodyBase.h"

#include "PhysicsConstraintActor.generated.h"

namespace ue5parse::runtime {

class UPhysicsConstraintComponent;

UCLASS()
class APhysicsConstraintActor : public ARigidBodyBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UPhysicsConstraintComponent> ConstraintComp = nullptr;

    explicit APhysicsConstraintActor(const FObjectInitializer& ObjectInitializer);
};

}
