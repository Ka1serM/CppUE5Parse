#include "GameTypes/FinalFantasy7Rebirth/EndTextResource.h"

namespace ue5parse::runtime {

void UEndTextResource::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    const int32 Count = Ar.Read<int32>();
    for (int32 i = 0; i < Count && !Ar.IsError(); i++) {
        FString Key;
        Ar << Key;
        //! Every key of an EndTextResource starts with a dollar sign.
        if (Key.IsEmpty() || Key[0] != '$') {
            Ar.SetError(ErrorCode::InvalidData, "UEndTextResource.Key");
            return;
        }
        Ar << Strings.Add(Key);
    }
}

}
