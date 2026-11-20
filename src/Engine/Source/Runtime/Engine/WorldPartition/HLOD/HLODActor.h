#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "HLODActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AWorldPartitionHLOD : public AActor {
    GENERATED_BODY()
public:
    explicit AWorldPartitionHLOD(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    uint32 LODLevel = 0;
    UPROPERTY()
    bool bRequireWarmup = false;
    UPROPERTY()
    FGuid SourceCellGuid;
};

}
