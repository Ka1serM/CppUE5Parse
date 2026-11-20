#include "Engine/Source/Runtime/Engine/ActorPartition/PartitionActor.h"

#include "Engine/Source/Runtime/Engine/Components/SceneComponent.h"

namespace ue5parse::runtime {

APartitionActor::APartitionActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    RootComponent = ObjectInitializer.CreateDefaultSubobject<USceneComponent>(FName("RootComponent0"));
    RootComponent->Mobility = EComponentMobility::Static;
}

}
