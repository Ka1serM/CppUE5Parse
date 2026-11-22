#pragma once

#include "Engine/Source/Runtime/Engine/GameFramework/Actor.h"

#include "MercunaNavDataChunk.generated.h"

namespace ue5parse::runtime {

//! Subnautica 2 saves a chunk of Mercuna navigation data before the properties of the actor.
UCLASS()
class AMercunaNavDataChunk : public AActor {
    GENERATED_BODY()
public:
    explicit AMercunaNavDataChunk(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

    void Serialize(FArchive& Ar) override {
        if (Ar.GetGame() == EGame::GAME_Subnautica2) {
            Ar.SkipBytes(8);
            Ar.SkipBytes(Ar.Read<int32>());
        }
        Super::Serialize(Ar);
    }
};

USTRUCT()
struct FUWEWorldPopSpatialCell {
    GENERATED_BODY()
    UPROPERTY()
    bool IsEmpty = false;
    UPROPERTY()
    bool IsPlayableSpace = false;
    UPROPERTY()
    bool IsLandscapeCell = false;

    friend FArchive& operator<<(FArchive& Ar, FUWEWorldPopSpatialCell& Cell) {
        Cell.IsEmpty = Ar.Read<uint8>() != 0;
        Cell.IsPlayableSpace = Ar.Read<uint8>() != 0;
        Cell.IsLandscapeCell = Ar.Read<uint8>() != 0;
        return Ar;
    }
};

USTRUCT()
struct FUWEWorldPopSpatialLayer {
    GENERATED_BODY()
    UPROPERTY()
    TMap<int64, FUWEWorldPopSpatialCell> CellMap;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Subnautica2)
            return false;
        Ar << CellMap;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FUWEWorldPopSpatialLayer> : TStructOpsTypeTraitsBase2<FUWEWorldPopSpatialLayer> {
    enum { WithSerializer = true };
};

}
