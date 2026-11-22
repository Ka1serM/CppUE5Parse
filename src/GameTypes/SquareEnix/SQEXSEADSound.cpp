#include "GameTypes/SquareEnix/SQEXSEADSound.h"

namespace ue5parse::runtime {

bool SerializeSQEXSEADData(FArchive& Ar, int32& Flags, FByteBulkData& Data, int32 HeaderSizeOfFlags17) {
    Ar << Flags;
    switch (Flags) {
    case 1:
        Ar.SkipBytes(24);
        break;
    case 3:
        Ar.SkipBytes(28);
        break;
    case 17:
        Ar.SkipBytes(HeaderSizeOfFlags17);
        break;
    default:
        Ar.SetError(ErrorCode::InvalidData, "SQEXSEAD.Flags");
        return false;
    }
    Data.Serialize(Ar);
    return !Ar.IsError();
}

void USQEXSEADSound::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    bool bHasData = false;
    Ar << bHasData;
    if (bHasData)
        SerializeSQEXSEADData(Ar, SQEXFlags, SQEXSoundData, 8);
}

void USQEXSEADSoundBank::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError())
        return;
    bool bHasData = Ar.GetGame() == EGame::GAME_DragonQuestXI;
    if (!bHasData)
        Ar << bHasData;
    if (bHasData)
        SerializeSQEXSEADData(Ar, SQEXFlags, SQEXSoundBankData, 4);
}

}
