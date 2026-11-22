#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "ItemDataContainer.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FItemDataContainer {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_VEIN)
            return false;
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FItemDataContainer> : TStructOpsTypeTraitsBase2<FItemDataContainer> {
    enum { WithSerializer = true };
};

}
