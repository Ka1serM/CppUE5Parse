#pragma once

#include "Engine/Source/Runtime/CoreUObject/Misc/WorldCompositionUtility.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"

#include "WorldComposition.generated.h"

namespace ue5parse::runtime {

class ULevelStreaming;

struct FWorldCompositionTile {
    FName PackageName;
    TArray<FName> LODPackageNames;
    FWorldTileInfo Info;
    double StreamingLevelStateChangeTime = 0.0;

    friend FArchive& operator<<(FArchive& Ar, FWorldCompositionTile& D) { return Ar << D.PackageName << D.Info << D.LODPackageNames; }
};

UCLASS()
class UWorldComposition : public UObject {
    GENERATED_BODY()
public:
    using FTilesList = TArray<FWorldCompositionTile>;

    void Serialize(FArchive& Ar) override;

    const FTilesList& GetTilesList() const { return Tiles; }
    const FString& GetWorldRoot() const { return WorldRoot; }

private:
    FString WorldRoot;
    FTilesList Tiles;

public:
    UPROPERTY(Transient)
    TArray<TObjectPtr<ULevelStreaming>> TilesStreaming;

    UPROPERTY()
    double TilesStreamingTimeThreshold = 1.0;

    UPROPERTY()
    bool bLoadAllTilesDuringCinematic = false;

    UPROPERTY()
    bool bRebaseOriginIn3DSpace = false;

    UPROPERTY()
    bool bLockTilesLocation = false;

    UPROPERTY()
    float RebaseOriginDistance = 0.0f;
};

}
