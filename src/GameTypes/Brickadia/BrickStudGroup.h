#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "BrickStudGroup.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FBrickStudGroup {
    GENERATED_BODY()
    UPROPERTY()
    FIntVector Offset;
    UPROPERTY()
    uint8 NumX = 0;
    UPROPERTY()
    uint8 NumY = 0;
    UPROPERTY()
    uint8 Direction = 0;
    UPROPERTY()
    uint8 Type = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Brickadia)
            return false;
        Ar << Offset << NumX << NumY << Direction << Type;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FBrickStudGroup> : TStructOpsTypeTraitsBase2<FBrickStudGroup> {
    enum { WithSerializer = true };
};

//! Brickadia saves its GUID as the four words of an engine GUID.
USTRUCT()
struct FBRGuid {
    GENERATED_BODY()
    UPROPERTY()
    FGuid Guid;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Brickadia)
            return false;
        Ar << Guid;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FBRGuid> : TStructOpsTypeTraitsBase2<FBRGuid> {
    enum { WithSerializer = true };
};

}
