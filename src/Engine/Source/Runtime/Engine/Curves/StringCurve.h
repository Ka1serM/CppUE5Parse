#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "StringCurve.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FStringCurveKey {
    GENERATED_BODY()
    UPROPERTY()
    float Time = 0;
    UPROPERTY()
    FString Value;

    bool Serialize(FArchive& Ar) {
        Ar << Time << Value;
        return true;
    }
    bool operator==(const FStringCurveKey& Other) const { return Time == Other.Time && Value.Equals(Other.Value); }
};

template <>
struct TStructOpsTypeTraits<FStringCurveKey> : TStructOpsTypeTraitsBase2<FStringCurveKey> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
