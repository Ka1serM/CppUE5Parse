#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "EncVector.generated.h"

namespace ue5parse::runtime {

//! Delta Force saves the encoded vector as three plain floats.
USTRUCT()
struct FEncVector {
    GENERATED_BODY()
    UPROPERTY()
    FVector3f Value;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_DeltaForce)
            return false;
        Ar << Value;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FEncVector> : TStructOpsTypeTraitsBase2<FEncVector> {
    enum { WithSerializer = true };
};

}
