#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ReanimalStructs.generated.h"

namespace ue5parse::runtime {

//! Reanimal saves only the raw value of the second property.
USTRUCT()
struct FEnumName {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Reanimal)
            return false;
        constexpr FRawPropertyRange Ranges[] = {{1, 1}};
        StaticStruct()->SerializeRawProperties(Ar, this, Ranges, RAW_RawValues);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FEnumName> : TStructOpsTypeTraitsBase2<FEnumName> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FAbstractEnum {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Reanimal)
            return false;
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FAbstractEnum> : TStructOpsTypeTraitsBase2<FAbstractEnum> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FJumpParams {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Reanimal)
            return false;
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FJumpParams> : TStructOpsTypeTraitsBase2<FJumpParams> {
    enum { WithSerializer = true };
};

//! Reanimal leaves out the third and fourth property of the key.
USTRUCT()
struct FChalkboardSectionKey {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Reanimal)
            return false;
        constexpr FRawPropertyRange Ranges[] = {{0, 2}, {2, 2}};
        StaticStruct()->SerializeRawProperties(Ar, this, Ranges, RAW_RawValues);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FChalkboardSectionKey> : TStructOpsTypeTraitsBase2<FChalkboardSectionKey> {
    enum { WithSerializer = true };
};

//! Reanimal saves the ledge metadata as one byte.
USTRUCT()
struct FLedgeMetaData {
    GENERATED_BODY()
    UPROPERTY()
    uint8 Data = 0;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Reanimal)
            return false;
        Ar << Data;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FLedgeMetaData> : TStructOpsTypeTraitsBase2<FLedgeMetaData> {
    enum { WithSerializer = true };
};

}
