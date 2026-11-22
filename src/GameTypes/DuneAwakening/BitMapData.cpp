#include "GameTypes/DuneAwakening/BitMapData.h"

namespace ue5parse::runtime {

void UBitMapData::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.GetGame() != EGame::GAME_DuneAwakening)
        return;
    const int32 NumBits = Ar.Read<int32>();
    if (NumBits < 0) {
        Ar.SetError(ErrorCode::InvalidData, "UBitMapData.NumBits");
        return;
    }
    const int32 NumBytes = NumBits >> 3;
    if (NumBytes > Ar.GetRemaining()) {
        Ar.SetError(ErrorCode::ReadPastEnd, "UBitMapData.Data");
        return;
    }
    Data.AddUninitialized(NumBytes);
    Ar.Serialize(Data.GetData(), NumBytes);
}

}
