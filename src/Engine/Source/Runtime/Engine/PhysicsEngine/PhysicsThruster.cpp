#include "Engine/Source/Runtime/Engine/PhysicsEngine/PhysicsThruster.h"

#include "Engine/Source/Runtime/Engine/PhysicsEngine/PhysicsThrusterComponent.h"

namespace ue5parse::runtime {

APhysicsThruster::APhysicsThruster(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    ThrusterComponent = ObjectInitializer.CreateDefaultSubobject<UPhysicsThrusterComponent>(FName("Thruster0"));
    RootComponent = ThrusterComponent;
}

}
