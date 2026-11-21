#pragma once

#include "Engine/Source/Runtime/CoreUObject/StructUtils/InstancedStruct.h"

#include "UniversalObjectLocatorFragment.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FUniversalObjectLocatorEmptyPayload {
    GENERATED_BODY()
};

USTRUCT()
struct FUniversalObjectLocatorFragment {
    GENERATED_BODY()

    FName FragmentTypeID;
    FInstancedStruct Payload;

    bool Serialize(FArchive& Ar);
    bool operator==(const FUniversalObjectLocatorFragment& Other) const { return FragmentTypeID == Other.FragmentTypeID && Payload == Other.Payload; }
};

template <>
struct TStructOpsTypeTraits<FUniversalObjectLocatorFragment> : TStructOpsTypeTraitsBase2<FUniversalObjectLocatorFragment> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
