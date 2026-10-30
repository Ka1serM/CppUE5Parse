#pragma once

#include "ue5parse/runtime/Engine/Curves/RealCurve.h"

#include "RichCurve.generated.h"

namespace ue5parse::runtime {

// FRichCurveKey: a key with its interpolation and tangents. Cooked data writes it natively (27 bytes).
USTRUCT()
struct FRichCurveKey {
    GENERATED_BODY()
    UPROPERTY()
    TEnumAsByte<ERichCurveInterpMode> InterpMode = RCIM_Linear;
    UPROPERTY()
    TEnumAsByte<ERichCurveTangentMode> TangentMode = RCTM_Auto;
    UPROPERTY()
    TEnumAsByte<ERichCurveTangentWeightMode> TangentWeightMode = RCTWM_WeightedNone;
    UPROPERTY()
    float Time = 0;
    UPROPERTY()
    float Value = 0;
    UPROPERTY()
    float ArriveTangent = 0;
    UPROPERTY()
    float ArriveTangentWeight = 0;
    UPROPERTY()
    float LeaveTangent = 0;
    UPROPERTY()
    float LeaveTangentWeight = 0;

    FRichCurveKey() = default;
    FRichCurveKey(float InTime, float InValue) : Time(InTime), Value(InValue) {}
    bool Serialize(FArchive& Ar);
    bool operator==(const FRichCurveKey& Other) const;
};

template <>
struct TStructOpsTypeTraits<FRichCurveKey> : TStructOpsTypeTraitsBase2<FRichCurveKey> {
    enum { WithSerializer = true, WithIdenticalViaEquality = true };
};

// FRichCurve: keys with linear, constant or cubic (weighted Bezier) interpolation (UE's Curves/RichCurve.h).
USTRUCT()
struct FRichCurve : public FRealCurve {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FRichCurveKey> Keys;

    int32 GetNumKeys() const override { return Keys.Num(); }
    bool IsEmpty() const { return Keys.Num() == 0; }
    const TArray<FRichCurveKey>& GetConstRefOfKeys() const { return Keys; }
    FRichCurveKey GetFirstKey() const { return Keys.Num() ? Keys[0] : FRichCurveKey(); }
    FRichCurveKey GetLastKey() const { return Keys.Num() ? Keys.Last() : FRichCurveKey(); }
    void GetTimeRange(float& MinTime, float& MaxTime) const;
    void GetValueRange(float& MinValue, float& MaxValue) const;
    bool HasAnyData() const { return DefaultValue != UE_MAX_FLT || Keys.Num() > 0; }
    // Adds a key in time order; returns its index.
    int32 AddKey(float InTime, float InValue);
    void Reset() { Keys.Empty(); }
    float Eval(float InTime, float InDefaultValue = 0.0f) const override;
    bool operator==(const FRichCurve& Other) const;

private:
    void RemapTimeValue(float& InTime, float& CycleValueOffset) const;
};

// How the keys of a compressed rich curve are stored.
enum ERichCurveCompressionFormat : int32 { RCCF_Empty, RCCF_Constant, RCCF_Linear, RCCF_Cubic, RCCF_Mixed, RCCF_Weighted, RCCF_Unknown };
enum ERichCurveKeyTimeCompressionFormat : int32 { RCKTCF_uint16, RCKTCF_float32 };

// FCompressedRichCurve: a rich curve whose keys are packed into bytes (UE's Curves/RichCurve.h). Cooked data writes
// it natively.
USTRUCT()
struct FCompressedRichCurve {
    GENERATED_BODY()
    TEnumAsByte<ERichCurveCompressionFormat> CompressionFormat = RCCF_Empty;
    TEnumAsByte<ERichCurveKeyTimeCompressionFormat> KeyTimeCompressionFormat = RCKTCF_float32;
    TEnumAsByte<ERichCurveExtrapolation> PreInfinityExtrap = RCCE_None;
    TEnumAsByte<ERichCurveExtrapolation> PostInfinityExtrap = RCCE_None;
    // The constant value of a constant curve, else the key count (UE's union).
    union {
        float ConstantValue;
        int32 NumKeys = 0;
    } ConstantValueNumKeys;
    TArray<uint8> CompressedKeys;

    bool Serialize(FArchive& Ar);
    float Eval(float InTime, float InDefaultValue = 0.0f) const;
    // The value of packed keys (UE's StaticEval). ConstantValueNumKeys is the constant of a constant or empty curve, else
    // the key count.
    static float StaticEval(ERichCurveCompressionFormat CompressionFormat, ERichCurveKeyTimeCompressionFormat KeyTimeCompressionFormat,
                            ERichCurveExtrapolation PreInfinityExtrap, ERichCurveExtrapolation PostInfinityExtrap,
                            int32 ConstantValueNumKeys, const uint8* CompressedKeys, float InTime, float InDefaultValue = 0.0f);
};

template <>
struct TStructOpsTypeTraits<FCompressedRichCurve> : TStructOpsTypeTraitsBase2<FCompressedRichCurve> {
    enum { WithSerializer = true };
};

namespace Curves {
// The value between two keys of a rich curve (UE's UE::Curves::EvalForTwoKeys): constant, linear, or the cubic Bezier
// of the tangents (weighted when a key has tangent weights).
float EvalForTwoKeys(const FRichCurveKey& Key1, const FRichCurveKey& Key2, float InTime);
}

}
