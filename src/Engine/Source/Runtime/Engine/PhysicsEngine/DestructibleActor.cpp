#include "Engine/Source/Runtime/Engine/PhysicsEngine/DestructibleActor.h"

#include "Engine/Source/Runtime/Engine/Components/DestructibleComponent.h"

namespace ue5parse::runtime {

ADestructibleActor::ADestructibleActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DestructibleComponent = ObjectInitializer.CreateDefaultSubobject<UDestructibleComponent>(FName("DestructibleComponent0"));
    RootComponent = DestructibleComponent;
}

}
