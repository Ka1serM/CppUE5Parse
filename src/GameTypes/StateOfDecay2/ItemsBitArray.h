#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ItemsBitArray.generated.h"

namespace ue5parse::runtime {

//! State of Decay 2 saves the number of bits as 16 bits and then the bits in whole bytes.
USTRUCT()
struct FItemsBitArray {
    GENERATED_BODY()
    UPROPERTY()
    uint16 NumBits = 0;
    UPROPERTY()
    TArray<uint8> Data;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_StateOfDecay2)
            return false;
        Ar << NumBits;
        const int32 NumBytes = (NumBits + 7) / 8;
        if (NumBytes > Ar.GetRemaining()) {
            Ar.SetError(ErrorCode::ReadPastEnd, "FItemsBitArray");
            return true;
        }
        Data.SetNum(NumBytes);
        Ar.Serialize(Data.GetData(), NumBytes);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FItemsBitArray> : TStructOpsTypeTraitsBase2<FItemsBitArray> {
    enum { WithSerializer = true };
};

}
