#include "Engine/Source/Runtime/Engine/PhysicsEngine/ClusterUnionActor.h"

#include "Engine/Source/Runtime/Engine/PhysicsEngine/ClusterUnionComponent.h"

namespace ue5parse::runtime {

AClusterUnionActor::AClusterUnionActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    ClusterUnion = ObjectInitializer.CreateDefaultSubobject<UClusterUnionComponent>(FName("ClusterUnion"));
    RootComponent = ClusterUnion;
}

}
