#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "DynamicMeshActor.generated.h"

namespace ue5parse::runtime {

class UDynamicMeshComponent;
class UDynamicMeshPool;

UCLASS()
class ADynamicMeshActor : public AActor {
    GENERATED_BODY()
public:
    explicit ADynamicMeshActor(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UDynamicMeshComponent> DynamicMeshComponent = nullptr;
    UPROPERTY()
    bool bEnableComputeMeshPool = true;
    UPROPERTY()
    TObjectPtr<UDynamicMeshPool> DynamicMeshPool = nullptr;
};

}
