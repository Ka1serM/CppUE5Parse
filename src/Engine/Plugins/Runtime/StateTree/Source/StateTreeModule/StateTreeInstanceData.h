#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"
#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStructContainer.h"

#include "StateTreeInstanceData.generated.h"

namespace ue5parse::runtime {

struct FStateTreeInstanceStorageCustomVersion {
    static constexpr FGuid GUID{0x60C4F0DE, 0x8B264C34, 0xAA937201, 0x5DFF09CC};
    enum Type : int32 {
        BeforeCustomVersionWasAdded = 0,
        AddedCustomSerialization,
    };
};

USTRUCT()
struct FStateTreeInstanceStorage {
    GENERATED_BODY()

    UPROPERTY()
    FInstancedStructContainer InstanceStructs;
};

USTRUCT()
struct FStateTreeInstanceData {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar);

    //! UE 5.3 reflects this property; from UE 5.4 it is the native storage that Serialize reads.
    UPROPERTY()
    TInstancedStruct<FStateTreeInstanceStorage> InstanceStorage;
};

template <>
struct TStructOpsTypeTraits<FStateTreeInstanceData> : TStructOpsTypeTraitsBase2<FStateTreeInstanceData> {
    enum { WithSerializer = true };
};

}
