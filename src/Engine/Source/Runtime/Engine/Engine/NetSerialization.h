#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "NetSerialization.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FFastArraySerializerItem {
    GENERATED_BODY()
    UPROPERTY()
    int32 ReplicationID = INDEX_NONE;
    UPROPERTY()
    int32 ReplicationKey = INDEX_NONE;
    UPROPERTY()
    int32 MostRecentArrayReplicationKey = INDEX_NONE;
};

UENUM()
enum class EFastArraySerializerDeltaFlags : uint8 {
    None,
    HasBeenSerialized = 1 << 0,
    HasDeltaBeenRequested = 1 << 1,
    IsUsingDeltaSerialization = 1 << 2,
};

USTRUCT()
struct FFastArraySerializer {
    GENERATED_BODY()
    UPROPERTY()
    int32 ArrayReplicationKey = 0;
    UPROPERTY(Transient)
    EFastArraySerializerDeltaFlags DeltaFlags = EFastArraySerializerDeltaFlags::None;
};

}
