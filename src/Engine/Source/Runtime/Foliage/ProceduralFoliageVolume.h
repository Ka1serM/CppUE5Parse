#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Volume.h"

#include "ProceduralFoliageVolume.generated.h"

namespace ue5parse::runtime {

class UProceduralFoliageComponent;

UCLASS()
class AProceduralFoliageVolume : public AVolume {
    GENERATED_BODY()
public:
    explicit AProceduralFoliageVolume(const FObjectInitializer& ObjectInitializer);

    UPROPERTY()
    TObjectPtr<UProceduralFoliageComponent> ProceduralComponent = nullptr;
};

}
