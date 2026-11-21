#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStructContainer.h"

#include "StateTreeInstanceData.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FStateTreeInstanceStorage {
    GENERATED_BODY()
    UPROPERTY()
    FInstancedStructContainer InstanceStructs;
};

USTRUCT()
struct FStateTreeInstanceData {
    GENERATED_BODY()
    UPROPERTY()
    FInstancedStruct InstanceStorage;

    bool Serialize(FArchive& Ar);
};

template <>
struct TStructOpsTypeTraits<FStateTreeInstanceData> : TStructOpsTypeTraitsBase2<FStateTreeInstanceData> {
    enum { WithSerializer = true };
};

}
