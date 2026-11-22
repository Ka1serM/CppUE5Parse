#pragma once

#include "Engine/Source/Runtime/Engine/Engine/DataAsset.h"

#include "BiomesSpawnPointsGrid.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FBiomesSpawnPoints {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FVector2f> Points;

    friend FArchive& operator<<(FArchive& Ar, FBiomesSpawnPoints& SpawnPoints) {
        BulkSerialize(Ar, SpawnPoints.Points);
        return Ar;
    }
};

UCLASS()
class UBiomesSpawnPointsGrid : public UDataAsset {
    GENERATED_BODY()
public:
    explicit UBiomesSpawnPointsGrid(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override {
        Super::Serialize(Ar);
        if (!Ar.IsError())
            Ar << SpawnPoints;
    }

    UPROPERTY()
    TArray<FBiomesSpawnPoints> SpawnPoints;
};

}
