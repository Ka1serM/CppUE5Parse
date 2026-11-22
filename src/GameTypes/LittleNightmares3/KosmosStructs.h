#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "KosmosStructs.generated.h"

namespace ue5parse::runtime {

//! Little Nightmares 3 saves only the raw value of the second property.
USTRUCT()
struct FSoftEnumName {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_LittleNightmares3)
            return false;
        constexpr FRawPropertyRange Ranges[] = {{1, 1}};
        StaticStruct()->SerializeRawProperties(Ar, this, Ranges, RAW_RawValues);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FSoftEnumName> : TStructOpsTypeTraitsBase2<FSoftEnumName> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FKosmosHangTraversalData {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_LittleNightmares3)
            return false;
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FKosmosHangTraversalData> : TStructOpsTypeTraitsBase2<FKosmosHangTraversalData> {
    enum { WithSerializer = true };
};

}
