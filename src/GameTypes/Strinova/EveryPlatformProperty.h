#pragma once

#include "Engine/Source/Runtime/Core/Containers/Map.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "EveryPlatformProperty.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FEveryPlatformFloat {
    GENERATED_BODY()
    UPROPERTY()
    float DefaultValue = 0;
    UPROPERTY()
    TMap<FName, float> PlatformOverrides;

    bool Serialize(FArchive& Ar) {
        Ar << DefaultValue << PlatformOverrides;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FEveryPlatformFloat> : TStructOpsTypeTraitsBase2<FEveryPlatformFloat> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FEveryPlatformBool {
    GENERATED_BODY()
    UPROPERTY()
    bool DefaultValue = false;
    UPROPERTY()
    TMap<FName, bool> PlatformOverrides;

    bool Serialize(FArchive& Ar) {
        Ar << DefaultValue << PlatformOverrides;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FEveryPlatformBool> : TStructOpsTypeTraitsBase2<FEveryPlatformBool> {
    enum { WithSerializer = true };
};

USTRUCT()
struct FEveryPlatformInt {
    GENERATED_BODY()
    UPROPERTY()
    int32 DefaultValue = 0;
    UPROPERTY()
    TMap<FName, int32> PlatformOverrides;

    bool Serialize(FArchive& Ar) {
        Ar << DefaultValue << PlatformOverrides;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FEveryPlatformInt> : TStructOpsTypeTraitsBase2<FEveryPlatformInt> {
    enum { WithSerializer = true };
};

}
