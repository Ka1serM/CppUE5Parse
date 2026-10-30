#pragma once

#include "ue5parse/runtime/Engine/Curves/RealCurve.h"

#include "SimpleCurve.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSimpleCurveKey {
    GENERATED_BODY()
    UPROPERTY()
    float Time = 0;
    UPROPERTY()
    float Value = 0;

    bool Serialize(FArchive& Ar);
    bool operator==(const FSimpleCurveKey& Other) const { return Time == Other.Time && Value == Other.Value; }
};

template <>
struct TStructOpsTypeTraits<FSimpleCurveKey> : TStructOpsTypeTraitsBase2<FSimpleCurveKey> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

// FSimpleCurve: keys with one interpolation for the whole curve, linear or constant (UE's Curves/SimpleCurve.h).
USTRUCT()
struct FSimpleCurve : public FRealCurve {
    GENERATED_BODY()
    UPROPERTY()
    TEnumAsByte<ERichCurveInterpMode> InterpMode = RCIM_Linear;
    UPROPERTY()
    TArray<FSimpleCurveKey> Keys;

    int32 GetNumKeys() const override { return Keys.Num(); }
    const TArray<FSimpleCurveKey>& GetConstRefOfKeys() const { return Keys; }
    float Eval(float InTime, float InDefaultValue = 0.0f) const override;

private:
    void RemapTimeValue(float& InTime, float& CycleValueOffset) const;
    float EvalForTwoKeys(const FSimpleCurveKey& Key1, const FSimpleCurveKey& Key2, float InTime) const;
};

}
