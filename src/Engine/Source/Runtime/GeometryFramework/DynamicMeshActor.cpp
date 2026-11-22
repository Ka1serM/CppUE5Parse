#include "Engine/Source/Runtime/GeometryFramework/DynamicMeshActor.h"

#include "Engine/Source/Runtime/GeometryFramework/Components/DynamicMeshComponent.h"
#include "Engine/Source/Runtime/GeometryFramework/UDynamicMesh.h"

namespace ue5parse::runtime {

ADynamicMeshActor::ADynamicMeshActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    DynamicMeshComponent = ObjectInitializer.CreateDefaultSubobject<UDynamicMeshComponent>(FName("DynamicMeshComponent"));
    DynamicMeshComponent->Mobility = EComponentMobility::Movable;
    DynamicMeshComponent->bGenerateOverlapEvents = false;
    DynamicMeshComponent->BodyInstance.CollisionProfileName = FName("BlockAll");
    DynamicMeshComponent->CollisionType = CTF_UseDefault;
    RootComponent = DynamicMeshComponent;
}

}
