#include "Engine/Source/Runtime/Engine/PhysicsEngine/PhysicsConstraintActor.h"

#include "Engine/Source/Runtime/Engine/PhysicsEngine/PhysicsConstraintComponent.h"

namespace ue5parse::runtime {

APhysicsConstraintActor::APhysicsConstraintActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    ConstraintComp = ObjectInitializer.CreateDefaultSubobject<UPhysicsConstraintComponent>(FName("MyConstraintComp"));
    RootComponent = ConstraintComp;
    bHidden = true;
}

}
