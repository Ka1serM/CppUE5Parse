#pragma once

#include "Engine/Source/Runtime/Engine/AI/Navigation/NavDataGatheringMode.h"
#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "StaticMeshActor.generated.h"

namespace ue5parse::runtime {

class UStaticMeshComponent;

UCLASS()
class AStaticMeshActor : public AActor {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> StaticMeshComponent = nullptr;
    UPROPERTY()
    bool bStaticMeshReplicateMovement = false;
    UPROPERTY()
    EPhysicsReplicationMode StaticMeshPhysicsReplicationMode = EPhysicsReplicationMode::Default;
    UPROPERTY()
    ENavDataGatheringMode NavigationGeometryGatheringMode = ENavDataGatheringMode::Default;

    explicit AStaticMeshActor(const FObjectInitializer& ObjectInitializer);
};

}
