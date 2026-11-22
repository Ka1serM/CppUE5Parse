#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "LegoPartInstance.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FLegoConnectionPoint {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Lego2KDrive)
            return false;
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FLegoConnectionPoint> : TStructOpsTypeTraitsBase2<FLegoConnectionPoint> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FLegoPartInstance {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_Lego2KDrive)
            return false;
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FLegoPartInstance> : TStructOpsTypeTraitsBase2<FLegoPartInstance> {
    enum { WithSerializer = true };
};

}
