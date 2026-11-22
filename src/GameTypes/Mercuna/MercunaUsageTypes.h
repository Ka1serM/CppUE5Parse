#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "MercunaUsageTypes.generated.h"

namespace ue5parse::runtime {

//! Mercuna saves the usage types as one 32 bit mask.
USTRUCT()
struct FMercunaUsageTypes {
    GENERATED_BODY()
    UPROPERTY()
    uint32 Mask = 0;

    bool Serialize(FArchive& Ar) {
        Ar << Mask;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMercunaUsageTypes> : TStructOpsTypeTraitsBase2<FMercunaUsageTypes> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FMercunaNavUsageTypes {
    GENERATED_BODY()
    UPROPERTY()
    uint32 Mask = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_HighOnLife2)
            return false;
        Ar << Mask;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMercunaNavUsageTypes> : TStructOpsTypeTraitsBase2<FMercunaNavUsageTypes> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FMercunaPawnUsageFlags {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_HighOnLife2)
            return false;
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMercunaPawnUsageFlags> : TStructOpsTypeTraitsBase2<FMercunaPawnUsageFlags> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FMercunaUsageSpec {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMercunaUsageSpec> : TStructOpsTypeTraitsBase2<FMercunaUsageSpec> {
    enum { WithSerializer = true };
};

}
