#pragma once

#include "Engine/Source/Runtime/Engine/Components/ActorComponent.h"

#include "ProceduralFoliageComponent.generated.h"

namespace ue5parse::runtime {

class AVolume;
class UProceduralFoliageSpawner;

UCLASS()
class UProceduralFoliageComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UProceduralFoliageSpawner> FoliageSpawner = nullptr;
    UPROPERTY()
    float TileOverlap = 0.0f;
    UPROPERTY()
    TObjectPtr<AVolume> SpawningVolume = nullptr;
    UPROPERTY()
    FGuid ProceduralGuid;
};

}
