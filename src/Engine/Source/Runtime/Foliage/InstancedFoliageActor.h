#pragma once

#include "Engine/Source/Runtime/Engine/ISMPartition/ISMPartitionActor.h"
#include "Engine/Source/Runtime/Foliage/FoliageType.h"
#include "Engine/Source/Runtime/Foliage/InstancedFoliage.h"

#include "InstancedFoliageActor.generated.h"

namespace ue5parse::runtime {

UCLASS()
class AInstancedFoliageActor : public AISMPartitionActor {
    GENERATED_BODY()
public:
    explicit AInstancedFoliageActor(const FObjectInitializer& ObjectInitializer);

    TMap<TObjectPtr<UFoliageType>, FFoliageInfo> FoliageInfos;

    void Serialize(FArchive& Ar) override;
};

}
