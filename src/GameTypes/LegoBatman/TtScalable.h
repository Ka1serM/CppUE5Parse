#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "TtScalable.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FTtScalableShadowFloat {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_LEGOBatmanLegacyoftheDarkKnight)
            return false;
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FTtScalableShadowFloat> : TStructOpsTypeTraitsBase2<FTtScalableShadowFloat> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FTtScalableLightFloat {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_LEGOBatmanLegacyoftheDarkKnight)
            return false;
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FTtScalableLightFloat> : TStructOpsTypeTraitsBase2<FTtScalableLightFloat> {
    enum { WithSerializer = true };
};

//! LEGO Batman saves only the value of the first property, in its unversioned form.
USTRUCT()
struct FTtScalableShadowBool {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_LEGOBatmanLegacyoftheDarkKnight)
            return false;
        constexpr FRawPropertyRange Ranges[] = {{0, 1}};
        StaticStruct()->SerializeRawProperties(Ar, this, Ranges, RAW_None);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FTtScalableShadowBool> : TStructOpsTypeTraitsBase2<FTtScalableShadowBool> {
    enum { WithSerializer = true };
};

}
