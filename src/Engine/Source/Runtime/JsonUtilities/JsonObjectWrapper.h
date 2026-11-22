#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "JsonObjectWrapper.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FJsonObjectWrapper {
    GENERATED_BODY()
    UPROPERTY()
    FString JsonString;

    //! MindsEye saves the wrapper as its JSON string without a property list.
    bool Serialize(FArchive& Ar) {
        if (Ar.GetGame() != EGame::GAME_MindsEye)
            return false;
        Ar << JsonString;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FJsonObjectWrapper> : TStructOpsTypeTraitsBase2<FJsonObjectWrapper> {
    enum { WithSerializer = true };
};

}
