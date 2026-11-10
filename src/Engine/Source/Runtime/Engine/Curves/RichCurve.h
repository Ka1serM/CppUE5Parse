#pragma once

#include "Engine/Source/Runtime/Engine/Curves/RealCurve.h"

#include "RichCurve.generated.h"

namespace ue5parse::runtime {

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

USTRUCT()
struct FRichCurve : public FRealCurve {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FRichCurveKey> Keys;

    int32 GetNumKeys() const override { return Keys.Num(); }
    bool IsEmpty() const { return Keys.Num() == 0; }
    const TArray<FRichCurveKey>& GetConstRefOfKeys() const { return Keys; }
    void GetTimeRange(float& MinTime, float& MaxTime) const;
    void GetValueRange(float& MinValue, float& MaxValue) const;
    int32 AddKey(float InTime, float InValue);
    void Reset() { Keys.Empty(); }
    float Eval(float InTime, float InDefaultValue = 0.0f) const override;
    bool operator==(const FRichCurve& Other) const;

private:
    void RemapTimeValue(float& InTime, float& CycleValueOffset) const;
};

enum ERichCurveCompressionFormat : int32 { RCCF_Empty, RCCF_Constant, RCCF_Linear, RCCF_Cubic, RCCF_Mixed, RCCF_Weighted, RCCF_Unknown };
enum ERichCurveKeyTimeCompressionFormat : int32 { RCKTCF_uint16, RCKTCF_float32 };

USTRUCT()
struct FCompressedRichCurve {
    GENERATED_BODY()
    TEnumAsByte<ERichCurveCompressionFormat> CompressionFormat = RCCF_Empty;
    TEnumAsByte<ERichCurveKeyTimeCompressionFormat> KeyTimeCompressionFormat = RCKTCF_float32;
    TEnumAsByte<ERichCurveExtrapolation> PreInfinityExtrap = RCCE_None;
    TEnumAsByte<ERichCurveExtrapolation> PostInfinityExtrap = RCCE_None;
    union {
        float ConstantValue;
        int32 NumKeys = 0;
    } ConstantValueNumKeys;
    TArray<uint8> CompressedKeys;

    bool Serialize(FArchive& Ar);
    float Eval(float InTime, float InDefaultValue = 0.0f) const;
    static float StaticEval(ERichCurveCompressionFormat CompressionFormat, ERichCurveKeyTimeCompressionFormat KeyTimeCompressionFormat,
                            ERichCurveExtrapolation PreInfinityExtrap, ERichCurveExtrapolation PostInfinityExtrap, int32 ConstantValueNumKeys,
                            const uint8* CompressedKeys, float InTime, float InDefaultValue = 0.0f);
};

template <>
struct TStructOpsTypeTraits<FCompressedRichCurve> : TStructOpsTypeTraitsBase2<FCompressedRichCurve> {
    enum { WithSerializer = true };
};

namespace Curves {

float EvalForTwoKeys(const FRichCurveKey& Key1, const FRichCurveKey& Key2, float InTime);

}

}
