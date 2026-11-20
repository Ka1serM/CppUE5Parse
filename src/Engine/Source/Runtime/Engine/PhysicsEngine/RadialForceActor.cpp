#include "Engine/Source/Runtime/Engine/PhysicsEngine/RadialForceActor.h"

#include "Engine/Source/Runtime/Engine/PhysicsEngine/RadialForceComponent.h"

namespace ue5parse::runtime {

ARadialForceActor::ARadialForceActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    ForceComponent = ObjectInitializer.CreateDefaultSubobject<URadialForceComponent>(FName("ForceComponent0"));
    RootComponent = ForceComponent;
}

}
