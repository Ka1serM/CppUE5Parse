#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ParameterWrapperArray.generated.h"

namespace ue5parse::runtime {

//! Neverness to Everness saves only the raw value of the first property.
USTRUCT()
struct FParameterWrapperArray {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_NevernessToEverness)
            return false;
        constexpr FRawPropertyRange Ranges[] = {{0, 1}};
        StaticStruct()->SerializeRawProperties(Ar, this, Ranges, RAW_RawValues);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FParameterWrapperArray> : TStructOpsTypeTraitsBase2<FParameterWrapperArray> {
    enum { WithSerializer = true };
};

}
