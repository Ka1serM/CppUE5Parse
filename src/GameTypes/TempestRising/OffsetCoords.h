#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "OffsetCoords.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FOffsetCoords {
    GENERATED_BODY()
    UPROPERTY()
    float X = 0;
    UPROPERTY()
    float Y = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_TempestRising)
            return false;
        Ar << X << Y;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FOffsetCoords> : TStructOpsTypeTraitsBase2<FOffsetCoords> {
    enum { WithSerializer = true };
};

}
