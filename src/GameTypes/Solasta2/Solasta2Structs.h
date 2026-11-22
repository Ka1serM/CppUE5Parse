#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "Solasta2Structs.generated.h"

namespace ue5parse::runtime {

//! Solasta 2 leaves out the seventh property of the parameters.
USTRUCT()
struct FRulesetActorCreationParams {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Solasta2)
            return false;
        constexpr FRawPropertyRange Ranges[] = {{0, 6}, {1, -1}};
        StaticStruct()->SerializeRawProperties(Ar, this, Ranges, RAW_RawValues);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FRulesetActorCreationParams> : TStructOpsTypeTraitsBase2<FRulesetActorCreationParams> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FHexOffsetCoord {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Solasta2)
            return false;
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FHexOffsetCoord> : TStructOpsTypeTraitsBase2<FHexOffsetCoord> {
    enum { WithSerializer = true };
};

//! Solasta 2 saves the second property of the ID before the first.
USTRUCT()
struct FRulesetId {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Solasta2)
            return false;
        constexpr FRawPropertyRange Ranges[] = {{0, 1}, {1, 1}};
        StaticStruct()->SerializeRawProperties(Ar, this, Ranges, RAW_Reverse | RAW_RawValues);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FRulesetId> : TStructOpsTypeTraitsBase2<FRulesetId> {
    enum { WithSerializer = true };
};

//! Solasta 2 saves the hex cell as one 64 bit value.
USTRUCT()
struct FHexCell {
    GENERATED_BODY()
    UPROPERTY()
    uint64 Value = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Solasta2)
            return false;
        Ar << Value;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FHexCell> : TStructOpsTypeTraitsBase2<FHexCell> {
    enum { WithSerializer = true };
};

}
