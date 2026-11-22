#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ItemStack.generated.h"

namespace ue5parse::runtime {

//! Fatekeeper saves the item stack as 158 bytes without a property list.
USTRUCT()
struct FItemStack {
    GENERATED_BODY()
    UPROPERTY()
    TArray<uint8> Data;

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Fatekeeper)
            return false;
        Data.SetNum(158);
        Ar.Serialize(Data.GetData(), Data.Num());
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FItemStack> : TStructOpsTypeTraitsBase2<FItemStack> {
    enum { WithSerializer = true };
};

}
