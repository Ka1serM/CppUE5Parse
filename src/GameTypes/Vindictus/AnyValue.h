#pragma once

#include "GameTypes/OuterWorlds2/PropertyDataWithObjects.h"

#include "AnyValue.generated.h"

namespace ue5parse::runtime {

//! Vindictus saves an old version byte, the struct object and then the struct value.
USTRUCT()
struct FAnyValue {
    GENERATED_BODY()
    UPROPERTY()
    FInstancedStruct NonConstStruct;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FAnyValue> : TStructOpsTypeTraitsBase2<FAnyValue> {
    enum { WithSerializer = true };
};

}
