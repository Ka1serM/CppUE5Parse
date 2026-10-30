#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

#include "NameCurve.generated.h"

namespace ue5parse::runtime {

// FNameCurveKey: a name at a time (UE's Curves/NameCurve.h). Cooked data writes it natively.
USTRUCT()
struct FNameCurveKey {
    GENERATED_BODY()
    UPROPERTY()
    float Time = 0;
    UPROPERTY()
    FName Value;

    bool Serialize(FArchive& Ar) {
        Ar << Time << Value;
        return true;
    }
    bool operator==(const FNameCurveKey& Other) const { return Time == Other.Time && Value == Other.Value; }
};

template <>
struct TStructOpsTypeTraits<FNameCurveKey> : TStructOpsTypeTraitsBase2<FNameCurveKey> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

}
