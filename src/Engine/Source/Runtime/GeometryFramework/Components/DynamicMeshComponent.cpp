#include "Engine/Source/Runtime/GeometryFramework/Components/DynamicMeshComponent.h"

#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/Engine/PhysicsEngine/BodySetup.h"
#include "Engine/Source/Runtime/GeometryFramework/UDynamicMesh.h"

namespace ue5parse::runtime {

UDynamicMeshComponent::UDynamicMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    PrimaryComponentTick.bCanEverTick = false;
    BodyInstance.CollisionProfileName = FName("NoCollision");
    MeshObject = ObjectInitializer.CreateDefaultSubobject<UDynamicMesh>(FName("DynamicMesh"));
    if (GetRuntimeProvider().getGame() >= EGame::GAME_UE5_3)
        TangentsType = EDynamicMeshComponentTangentsMode::Default;
}

}
