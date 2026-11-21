#pragma once

#include "Engine/Source/Runtime/Engine/ActorPartition/PartitionActor.h"

#include "PCGPartitionActor.generated.h"

namespace ue5parse::runtime {

class UBoxComponent;

UCLASS()
class APCGPartitionActor : public APartitionActor {
    GENERATED_BODY()
public:
    explicit APCGPartitionActor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    UPROPERTY()
    FGuid PCGGuid;
    UPROPERTY()
    uint32 PCGGridSize = 0;
    UPROPERTY()
    bool bUse2DGrid = false;
    UPROPERTY()
    TObjectPtr<UBoxComponent> BoundsComponent = nullptr;
};

}
