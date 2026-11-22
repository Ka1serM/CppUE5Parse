#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "BHVRVariantConfigurator.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FBHVRVariantConfigurator {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_DeadByDaylight)
            return false;
        StaticStruct()->SerializeRawProperties(Ar, this);
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FBHVRVariantConfigurator> : TStructOpsTypeTraitsBase2<FBHVRVariantConfigurator> {
    enum { WithSerializer = true };
};

}
