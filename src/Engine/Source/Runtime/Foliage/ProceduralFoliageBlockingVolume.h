#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"
#include "Engine/Source/Runtime/Foliage/FoliageType.h"

#include "ProceduralFoliageBlockingVolume.generated.h"

namespace ue5parse::runtime {

class AProceduralFoliageVolume;

UCLASS()
class AProceduralFoliageBlockingVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit AProceduralFoliageBlockingVolume(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<AProceduralFoliageVolume> ProceduralFoliageVolume = nullptr;
    UPROPERTY()
    FFoliageDensityFalloff DensityFalloff;
};

}
