#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "DataLayerInstanceNames.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FDataLayerInstanceNames {
    GENERATED_BODY()

    UPROPERTY()
    bool bIsFirstDataLayerExternal = false;
    UPROPERTY()
    TArray<FName> DataLayers;

    //! Before 5.4 the cell stored its data layers as a plain TArray<FName> under the same property name.
    bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FDataLayerInstanceNames> : TStructOpsTypeTraitsBase2<FDataLayerInstanceNames> {
    enum { WithSerializeFromMismatchedTag = true };
};

}
