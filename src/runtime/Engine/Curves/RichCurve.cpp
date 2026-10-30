#include "ue5parse/runtime/Engine/Curves/RichCurve.h"

#include <cmath>
#include <cstring>
#include <limits>

namespace ue5parse::runtime {

namespace {

float BezierInterp(float P0, float P1, float P2, float P3, float Alpha) {
    const float P01 = FMath::Lerp(P0, P1, Alpha);
    const float P12 = FMath::Lerp(P1, P2, Alpha);
    const float P23 = FMath::Lerp(P2, P3, Alpha);
    const float P012 = FMath::Lerp(P01, P12, Alpha);
    const float P123 = FMath::Lerp(P12, P23, Alpha);
    return FMath::Lerp(P012, P123, Alpha);
}

// The real roots of a cubic in power basis (Cardano), as UE's UE::Curves::SolveCubic.
int32 SolveCubic(double Coeff[4], double Solution[3]) {
    auto Cbrt = [](double X) { return X > 0.0 ? std::pow(X, 1.0 / 3.0) : (X < 0.0 ? -std::pow(-X, 1.0 / 3.0) : 0.0); };
    int32 NumSolutions = 0;
    const double Denominator = Coeff[3] != 0.0 ? Coeff[3] : UE_DOUBLE_SMALL_NUMBER * UE_DOUBLE_SMALL_NUMBER;
    const double A = Coeff[2] / Denominator;
    const double B = Coeff[1] / Denominator;
    const double C = Coeff[0] / Denominator;
    const double SqOfA = A * A;
    const double P = 1.0 / 3 * (-1.0 / 3 * SqOfA + B);
    const double Q = 1.0 / 2 * (2.0 / 27 * A * SqOfA - 1.0 / 3 * A * B + C);
    const double CubeOfP = P * P * P;
    const double D = Q * Q + CubeOfP;
    if (FMath::IsNearlyZero(D)) {
        if (FMath::IsNearlyZero(Q)) {
            Solution[0] = 0;
            NumSolutions = 1;
        } else {
            const double U = Cbrt(-Q);
            Solution[0] = 2 * U;
            Solution[1] = -U;
            NumSolutions = 2;
        }
    } else if (D < 0) {
        const double Phi = 1.0 / 3 * std::acos(-Q / std::sqrt(-CubeOfP));
        const double T = 2 * std::sqrt(-P);
        Solution[0] = T * std::cos(Phi);
        Solution[1] = -T * std::cos(Phi + UE_PI / 3);
        Solution[2] = -T * std::cos(Phi - UE_PI / 3);
        NumSolutions = 3;
    } else {
        const double SqrtD = std::sqrt(D);
        Solution[0] = Cbrt(SqrtD - Q) - Cbrt(SqrtD + Q);
        NumSolutions = 1;
    }
    const double Sub = 1.0 / 3 * A;
    for (int32 Index = 0; Index < NumSolutions; ++Index) Solution[Index] -= Sub;
    return NumSolutions;
}

void BezierToPower(double A1, double B1, double C1, double D1, double* A2, double* B2, double* C2, double* D2) {
    const double A = B1 - A1;
    const double B = C1 - B1;
    const double C = D1 - C1;
    const double D = B - A;
    *A2 = C - B - D;
    *B2 = 3.0 * D;
    *C2 = 3.0 * A;
    *D2 = A1;
}

float WeightedEvalForTwoKeys(float Key1Value, float Key1Time, float Key1LeaveTangent, float Key1LeaveTangentWeight,
                             ERichCurveTangentWeightMode Key1TangentWeightMode, float Key2Value, float Key2Time, float Key2ArriveTangent,
                             float Key2ArriveTangentWeight, ERichCurveTangentWeightMode Key2TangentWeightMode, float InTime) {
    const float Diff = Key2Time - Key1Time;
    const float Alpha = (InTime - Key1Time) / Diff;
    const float OneThird = 1.0f / 3.0f;
    const double Time1 = Key1Time;
    const double Time2 = Key2Time;
    const float X = static_cast<float>(Time2 - Time1);
    float CosAngle, SinAngle;
    float Angle = std::atan(Key1LeaveTangent);
    FMath::SinCos(&SinAngle, &CosAngle, Angle);
    float LeaveWeight;
    if (Key1TangentWeightMode == RCTWM_WeightedNone || Key1TangentWeightMode == RCTWM_WeightedArrive) {
        const float Y = Key1LeaveTangent * X;
        LeaveWeight = FMath::Sqrt(X * X + Y * Y) * OneThird;
    } else {
        LeaveWeight = Key1LeaveTangentWeight;
    }
    const float Key1TanX = CosAngle * LeaveWeight + static_cast<float>(Time1);
    const float Key1TanY = SinAngle * LeaveWeight + Key1Value;
    Angle = std::atan(Key2ArriveTangent);
    FMath::SinCos(&SinAngle, &CosAngle, Angle);
    float ArriveWeight;
    if (Key2TangentWeightMode == RCTWM_WeightedNone || Key2TangentWeightMode == RCTWM_WeightedLeave) {
        const float Y = Key2ArriveTangent * X;
        ArriveWeight = FMath::Sqrt(X * X + Y * Y) * OneThird;
    } else {
        ArriveWeight = Key2ArriveTangentWeight;
    }
    const float Key2TanX = -CosAngle * ArriveWeight + static_cast<float>(Time2);
    const float Key2TanY = -SinAngle * ArriveWeight + Key2Value;
    const float RangeX = static_cast<float>(Time2 - Time1);
    const float NormalizedX1 = (Key1TanX - static_cast<float>(Time1)) / RangeX;
    const float NormalizedX2 = (Key2TanX - static_cast<float>(Time1)) / RangeX;
    double Coeff[4];
    double Results[3];
    BezierToPower(0.0, NormalizedX1, NormalizedX2, 1.0, &Coeff[3], &Coeff[2], &Coeff[1], &Coeff[0]);
    Coeff[0] = Coeff[0] - Alpha;
    const int32 NumResults = SolveCubic(Coeff, Results);
    double NewInterp = Alpha;
    if (NumResults == 1) {
        NewInterp = Results[0];
    } else {
        // The largest root in [0, 1]; UE also walks the unset entries of the array.
        NewInterp = std::numeric_limits<double>::lowest();
        for (int32 i = 0; i < NumResults; i++) {
            const double Result = Results[i];
            if (Result >= 0.0 && Result <= 1.0 && (NewInterp < 0.0 || Result > NewInterp)) NewInterp = Result;
        }
        if (NewInterp == std::numeric_limits<double>::lowest()) NewInterp = 0.0;
    }
    return BezierInterp(Key1Value, Key1TanY, Key2TanY, Key2Value, static_cast<float>(NewInterp));
}

bool IsItNotWeighted(const FRichCurveKey& Key1, const FRichCurveKey& Key2) {
    return (Key1.TangentWeightMode == RCTWM_WeightedNone || Key1.TangentWeightMode == RCTWM_WeightedArrive) &&
           (Key2.TangentWeightMode == RCTWM_WeightedNone || Key2.TangentWeightMode == RCTWM_WeightedLeave);
}

}

namespace Curves {

float EvalForTwoKeys(const FRichCurveKey& Key1, const FRichCurveKey& Key2, float InTime) {
    const float Diff = Key2.Time - Key1.Time;
    if (Diff <= 0.f) return Key1.Value;
    if (Key1.InterpMode == RCIM_Constant) return InTime < Key2.Time ? Key1.Value : Key2.Value;
    const float Alpha = (InTime - Key1.Time) / Diff;
    const float P0 = Key1.Value;
    const float P3 = Key2.Value;
    if (Key1.InterpMode == RCIM_Linear) return FMath::Lerp(P0, P3, Alpha);
    if (IsItNotWeighted(Key1, Key2)) {
        const float OneThird = 1.0f / 3.0f;
        const float P1 = P0 + (Key1.LeaveTangent * Diff * OneThird);
        const float P2 = P3 - (Key2.ArriveTangent * Diff * OneThird);
        return BezierInterp(P0, P1, P2, P3, Alpha);
    }
    return WeightedEvalForTwoKeys(Key1.Value, Key1.Time, Key1.LeaveTangent, Key1.LeaveTangentWeight, Key1.TangentWeightMode, Key2.Value,
                                  Key2.Time, Key2.ArriveTangent, Key2.ArriveTangentWeight, Key2.TangentWeightMode, InTime);
}

}

bool FRichCurveKey::Serialize(FArchive& Ar) {
    uint8 Interp = 0, Tangent = 0, TangentWeight = 0;
    Ar << Interp << Tangent << TangentWeight;
    InterpMode = static_cast<ERichCurveInterpMode>(Interp);
    TangentMode = static_cast<ERichCurveTangentMode>(Tangent);
    TangentWeightMode = static_cast<ERichCurveTangentWeightMode>(TangentWeight);
    Ar << Time << Value << ArriveTangent << ArriveTangentWeight << LeaveTangent << LeaveTangentWeight;
    return true;
}

bool FRichCurveKey::operator==(const FRichCurveKey& Other) const {
    return Time == Other.Time && Value == Other.Value && InterpMode == Other.InterpMode && TangentMode == Other.TangentMode &&
           TangentWeightMode == Other.TangentWeightMode &&
           (InterpMode != RCIM_Cubic || (ArriveTangent == Other.ArriveTangent && LeaveTangent == Other.LeaveTangent));
}

void FRichCurve::GetTimeRange(float& MinTime, float& MaxTime) const {
    if (Keys.Num() == 0) {
        MinTime = MaxTime = 0.f;
        return;
    }
    MinTime = Keys[0].Time;
    MaxTime = Keys.Last().Time;
}

namespace {

// UE's FeaturePointMethod: samples a cubic segment, and closer where the slope changes sign.
void FeaturePointMethod(const FRichCurve& Curve, float StartTime, float EndTime, float StartValue, float Mu, int32 Depth, int32 MaxDepth,
                        float& MaxV, float& MinVal) {
    if (Depth >= MaxDepth) return;
    float PrevValue = StartValue;
    float PrevTangent = StartValue - Curve.Eval(StartTime - Mu);
    EndTime += Mu;
    for (float F = StartTime + Mu; F < EndTime; F += Mu) {
        const float Value = Curve.Eval(F);
        MaxV = FMath::Max(Value, MaxV);
        MinVal = FMath::Min(Value, MinVal);
        const float CurTangent = Value - PrevValue;
        if (FMath::Sign(CurTangent) != FMath::Sign(PrevTangent)) {
            const float FeaturePointTime = F - Mu * 2.0f;
            FeaturePointMethod(Curve, FeaturePointTime, F, Curve.Eval(FeaturePointTime), Mu * 0.4f, Depth + 1, MaxDepth, MaxV, MinVal);
        }
        PrevTangent = CurTangent;
        PrevValue = Value;
    }
}

}

void FRichCurve::GetValueRange(float& MinValue, float& MaxValue) const {
    if (Keys.Num() == 0) {
        MinValue = MaxValue = 0.f;
        return;
    }
    const int32 LastKeyIndex = Keys.Num() - 1;
    MinValue = MaxValue = Keys[0].Value;
    for (int32 i = 0; i < Keys.Num(); i++) {
        const FRichCurveKey& Key = Keys[i];
        MinValue = FMath::Min(MinValue, Key.Value);
        MaxValue = FMath::Max(MaxValue, Key.Value);
        if (Key.InterpMode == RCIM_Cubic && i != LastKeyIndex) {
            const FRichCurveKey& NextKey = Keys[i + 1];
            const float TimeStep = (NextKey.Time - Key.Time) * 0.2f;
            FeaturePointMethod(*this, Key.Time, NextKey.Time, Key.Value, TimeStep, 0, 3, MaxValue, MinValue);
        }
    }
}

int32 FRichCurve::AddKey(float InTime, float InValue) {
    int32 Index = 0;
    while (Index < Keys.Num() && Keys[Index].Time < InTime) Index++;
    Keys.Insert(FRichCurveKey(InTime, InValue), Index);
    return Index;
}

void FRichCurve::RemapTimeValue(float& InTime, float& CycleValueOffset) const {
    const int32 NumKeys = Keys.Num();
    if (NumKeys < 2) return;
    if (InTime <= Keys[0].Time) {
        if (PreInfinityExtrap != RCCE_Linear && PreInfinityExtrap != RCCE_Constant) {
            const float MinTime = Keys[0].Time;
            const float MaxTime = Keys[NumKeys - 1].Time;
            int32 CycleCount = 0;
            CycleTime(MinTime, MaxTime, InTime, CycleCount);
            if (PreInfinityExtrap == RCCE_CycleWithOffset) {
                CycleValueOffset = (Keys[0].Value - Keys[NumKeys - 1].Value) * static_cast<float>(CycleCount);
            } else if (PreInfinityExtrap == RCCE_Oscillate && CycleCount % 2 == 1) {
                InTime = MinTime + (MaxTime - InTime);
            }
        }
    } else if (InTime >= Keys[NumKeys - 1].Time) {
        if (PostInfinityExtrap != RCCE_Linear && PostInfinityExtrap != RCCE_Constant) {
            const float MinTime = Keys[0].Time;
            const float MaxTime = Keys[NumKeys - 1].Time;
            int32 CycleCount = 0;
            CycleTime(MinTime, MaxTime, InTime, CycleCount);
            if (PostInfinityExtrap == RCCE_CycleWithOffset) {
                CycleValueOffset = (Keys[NumKeys - 1].Value - Keys[0].Value) * static_cast<float>(CycleCount);
            } else if (PostInfinityExtrap == RCCE_Oscillate && CycleCount % 2 == 1) {
                InTime = MinTime + (MaxTime - InTime);
            }
        }
    }
}

float FRichCurve::Eval(float InTime, float InDefaultValue) const {
    float CycleValueOffset = 0;
    RemapTimeValue(InTime, CycleValueOffset);
    const int32 NumKeys = Keys.Num();
    float InterpVal = DefaultValue == UE_MAX_FLT ? InDefaultValue : DefaultValue;
    if (NumKeys == 0) {
        // The default value.
    } else if (NumKeys < 2 || InTime < Keys[0].Time) {
        if (PreInfinityExtrap == RCCE_Linear && NumKeys > 1) {
            const float DT = Keys[1].Time - Keys[0].Time;
            if (FMath::IsNearlyZero(DT)) {
                InterpVal = Keys[0].Value;
            } else {
                const float Slope = (Keys[1].Value - Keys[0].Value) / DT;
                InterpVal = Slope * (InTime - Keys[0].Time) + Keys[0].Value;
            }
        } else {
            InterpVal = Keys[0].Value;
        }
    } else if (InTime <= Keys[NumKeys - 1].Time) {
        // Lower bound of the second key of the segment.
        int32 First = 1;
        int32 Count = NumKeys - 1 - First;
        while (Count > 0) {
            const int32 Step = Count / 2;
            const int32 Middle = First + Step;
            if (InTime >= Keys[Middle].Time) {
                First = Middle + 1;
                Count -= Step + 1;
            } else {
                Count = Step;
            }
        }
        InterpVal = Curves::EvalForTwoKeys(Keys[First - 1], Keys[First], InTime);
    } else {
        if (PostInfinityExtrap == RCCE_Linear) {
            const float DT = Keys[NumKeys - 2].Time - Keys[NumKeys - 1].Time;
            if (FMath::IsNearlyZero(DT)) {
                InterpVal = Keys[NumKeys - 1].Value;
            } else {
                const float Slope = (Keys[NumKeys - 2].Value - Keys[NumKeys - 1].Value) / DT;
                InterpVal = Slope * (InTime - Keys[NumKeys - 1].Time) + Keys[NumKeys - 1].Value;
            }
        } else {
            InterpVal = Keys[NumKeys - 1].Value;
        }
    }
    return InterpVal + CycleValueOffset;
}

bool FRichCurve::operator==(const FRichCurve& Other) const {
    return Keys == Other.Keys && PreInfinityExtrap == Other.PreInfinityExtrap && PostInfinityExtrap == Other.PostInfinityExtrap &&
           DefaultValue == Other.DefaultValue;
}

bool FCompressedRichCurve::Serialize(FArchive& Ar) {
    Ar << CompressionFormat << KeyTimeCompressionFormat << PreInfinityExtrap << PostInfinityExtrap;
    Ar << ConstantValueNumKeys.NumKeys;
    const int32 NumBytes = Ar.ReadCount(1);
    SerializeRaw(Ar, CompressedKeys, NumBytes);
    return true;
}


// The evaluation of packed keys (UE's RichCurve.cpp key adapters and InterpEval, UE 5.3).
namespace {

int32 AlignOffset(int32 Offset, int32 Alignment) { return (Offset + Alignment - 1) / Alignment * Alignment; }

template <class T>
T ReadPacked(const uint8* Base, int32 Offset) {
    T Value;
    std::memcpy(&Value, Base + Offset, sizeof(T));
    return Value;
}

// Key times: 16-bit fractions of a range, or floats. KeyDataOffset is where the key values start.
struct FKeyTimes {
    const uint8* Base = nullptr;
    int32 KeyTimesOffset = 0;
    int32 KeyDataOffset = 0;
    bool bQuantized = false;
    float MinTime = 0;
    float DeltaTime = 0;

    FKeyTimes(const uint8* InBase, int32 InKeyTimesOffset, int32 NumKeys, bool bInQuantized)
        : Base(InBase), KeyTimesOffset(InKeyTimesOffset), bQuantized(bInQuantized) {
        if (bQuantized) {
            const int32 RangeDataOffset = AlignOffset(KeyTimesOffset + NumKeys * static_cast<int32>(sizeof(uint16)), sizeof(float));
            KeyDataOffset = RangeDataOffset + 2 * sizeof(float);
            MinTime = ReadPacked<float>(Base, RangeDataOffset);
            DeltaTime = ReadPacked<float>(Base, RangeDataOffset + sizeof(float));
        } else {
            KeyDataOffset = AlignOffset(KeyTimesOffset + NumKeys * static_cast<int32>(sizeof(float)), sizeof(float));
        }
    }
    float GetTime(int32 KeyIndex) const {
        if (!bQuantized) return ReadPacked<float>(Base, KeyTimesOffset + KeyIndex * 4);
        return ReadPacked<uint16>(Base, KeyTimesOffset + KeyIndex * 2) * (1.0f / 65535.0f) * DeltaTime + MinTime;
    }
};

// Key values: one float (linear), value and tangents (cubic, mixed), or also the tangent weights (weighted), with the
// interpolation of each key for mixed and weighted curves.
struct FKeyData {
    const uint8* Base = nullptr;
    int32 KeyDataOffset = 0;
    int32 InterpModesOffset = 0;
    ERichCurveCompressionFormat Format = RCCF_Linear;

    int32 GetKeyDataHandle(int32 KeyIndex) const {
        if (Format == RCCF_Weighted) return KeyIndex * 5;
        if (Format == RCCF_Linear) return KeyIndex;
        return KeyIndex * 3;
    }
    float Get(int32 Handle) const { return ReadPacked<float>(Base, KeyDataOffset + Handle * 4); }
    float GetKeyValue(int32 Handle) const { return Get(Handle); }
    float GetKeyArriveTangent(int32 Handle) const { return Get(Handle + 1); }
    float GetKeyLeaveTangent(int32 Handle) const { return Get(Handle + 2); }
    float GetKeyArriveTangentWeight(int32 Handle) const { return Format == RCCF_Weighted ? Get(Handle + 3) : 0.0f; }
    float GetKeyLeaveTangentWeight(int32 Handle) const { return Format == RCCF_Weighted ? Get(Handle + 4) : 0.0f; }
    ERichCurveCompressionFormat GetKeyInterpMode(int32 KeyIndex) const {
        if (Format == RCCF_Mixed) return static_cast<ERichCurveCompressionFormat>(Base[InterpModesOffset + KeyIndex]);
        if (Format == RCCF_Weighted) return static_cast<ERichCurveCompressionFormat>(Base[InterpModesOffset + KeyIndex * 2]);
        return Format;
    }
    // Indexed as UE 5.3 calls it: InterpEval passes a key data handle, not a key index.
    ERichCurveTangentWeightMode GetKeyTangentWeightMode(int32 Index) const {
        if (Format != RCCF_Weighted) return RCTWM_WeightedNone;
        return static_cast<ERichCurveTangentWeightMode>(Base[InterpModesOffset + Index * 2 + 1]);
    }
};

void CycleTime(float MinTime, float MaxTime, float& InTime, int& CycleCount) {
    const float InitTime = InTime;
    const float Duration = MaxTime - MinTime;
    if (InTime > MaxTime) {
        CycleCount = static_cast<int>(std::floor((MaxTime - InTime) / Duration));
        InTime = InTime + Duration * CycleCount;
    } else if (InTime < MinTime) {
        CycleCount = static_cast<int>(std::floor((InTime - MinTime) / Duration));
        InTime = InTime - Duration * CycleCount;
    }
    if (InTime == MaxTime && InitTime < MinTime) InTime = MinTime;
    if (InTime == MinTime && InitTime > MaxTime) InTime = MaxTime;
    CycleCount = std::abs(CycleCount);
}

float RemapTimeValue(float InTime, const FKeyTimes& Times, const FKeyData& Data, int32 NumKeys, ERichCurveExtrapolation InfinityExtrap,
                     int32 KeyIndex0, int32 KeyIndex1, float& CycleValueOffset) {
    const float MinTime = Times.GetTime(0);
    const float MaxTime = Times.GetTime(NumKeys - 1);
    int CycleCount = 0;
    CycleTime(MinTime, MaxTime, InTime, CycleCount);
    if (InfinityExtrap == RCCE_CycleWithOffset) {
        CycleValueOffset = (Data.GetKeyValue(Data.GetKeyDataHandle(KeyIndex0)) - Data.GetKeyValue(Data.GetKeyDataHandle(KeyIndex1))) * CycleCount;
    } else if (InfinityExtrap == RCCE_Oscillate && CycleCount % 2 == 1) {
        InTime = MinTime + (MaxTime - InTime);
    }
    return InTime;
}

float InterpEvalExtrapolate(float InTime, const FKeyTimes& Times, const FKeyData& Data, ERichCurveExtrapolation InfinityExtrap, int32 KeyIndex0,
                            int32 KeyIndex1, float KeyTime0) {
    const float KeyValue0 = Data.GetKeyValue(Data.GetKeyDataHandle(KeyIndex0));
    if (InfinityExtrap != RCCE_Linear) return KeyValue0;
    const float DT = Times.GetTime(KeyIndex1) - KeyTime0;
    if (FMath::IsNearlyZero(DT)) return KeyValue0;
    const float DV = Data.GetKeyValue(Data.GetKeyDataHandle(KeyIndex1)) - KeyValue0;
    return DV / DT * (InTime - KeyTime0) + KeyValue0;
}

float InterpEval(float InTime, const FKeyTimes& Times, const FKeyData& Data, int32 NumKeys, ERichCurveExtrapolation PreInfinityExtrap,
                 ERichCurveExtrapolation PostInfinityExtrap) {
    float CycleValueOffset = 0;
    const float FirstKeyTime = Times.GetTime(0);
    if (InTime < FirstKeyTime) {
        if (PreInfinityExtrap != RCCE_Linear && PreInfinityExtrap != RCCE_Constant)
            InTime = RemapTimeValue(InTime, Times, Data, NumKeys, PreInfinityExtrap, 0, NumKeys - 1, CycleValueOffset);
        else
            return InterpEvalExtrapolate(InTime, Times, Data, PreInfinityExtrap, 0, 1, FirstKeyTime);
    }
    const float LastKeyTime = Times.GetTime(NumKeys - 1);
    if (InTime > LastKeyTime) {
        if (PostInfinityExtrap != RCCE_Linear && PostInfinityExtrap != RCCE_Constant)
            InTime = RemapTimeValue(InTime, Times, Data, NumKeys, PostInfinityExtrap, NumKeys - 1, 0, CycleValueOffset);
        else
            return InterpEvalExtrapolate(InTime, Times, Data, PostInfinityExtrap, NumKeys - 1, NumKeys - 2, LastKeyTime);
    }
    int32 First = 1;
    int32 Count = NumKeys - 1 - First;
    while (Count > 0) {
        const int32 Step = Count / 2;
        const int32 Middle = First + Step;
        if (InTime >= Times.GetTime(Middle)) {
            First = Middle + 1;
            Count -= Step + 1;
        } else {
            Count = Step;
        }
    }
    const float KeyTime0 = Times.GetTime(First - 1);
    const float KeyTime1 = Times.GetTime(First);
    const float Diff = KeyTime1 - KeyTime0;
    const int32 Handle0 = Data.GetKeyDataHandle(First - 1);
    const float KeyValue0 = Data.GetKeyValue(Handle0);
    const ERichCurveCompressionFormat KeyInterpMode0 = Data.GetKeyInterpMode(First - 1);
    if (!(Diff > 0.0f && KeyInterpMode0 != RCCF_Constant)) return KeyValue0 + CycleValueOffset;
    const int32 Handle1 = Data.GetKeyDataHandle(First);
    const float KeyValue1 = Data.GetKeyValue(Handle1);
    const float Alpha = (InTime - KeyTime0) / Diff;
    if (KeyInterpMode0 == RCCF_Linear) return FMath::Lerp(KeyValue0, KeyValue1, Alpha) + CycleValueOffset;
    const ERichCurveTangentWeightMode Mode0 = Data.GetKeyTangentWeightMode(Handle0);
    const ERichCurveTangentWeightMode Mode1 = Data.GetKeyTangentWeightMode(Handle1);
    if ((Mode0 == RCTWM_WeightedNone || Mode0 == RCTWM_WeightedArrive) && (Mode1 == RCTWM_WeightedNone || Mode1 == RCTWM_WeightedLeave)) {
        const float ScaledDiff = Diff * (1.0f / 3.0f);
        const float P1 = KeyValue0 + Data.GetKeyLeaveTangent(Handle0) * ScaledDiff;
        const float P2 = KeyValue1 - Data.GetKeyArriveTangent(Handle1) * ScaledDiff;
        return BezierInterp(KeyValue0, P1, P2, KeyValue1, Alpha) + CycleValueOffset;
    }
    return WeightedEvalForTwoKeys(KeyValue0, KeyTime0, Data.GetKeyLeaveTangent(Handle0), Data.GetKeyLeaveTangentWeight(Handle0),
                                  Data.GetKeyTangentWeightMode(First - 1), KeyValue1, KeyTime1, Data.GetKeyArriveTangent(Handle1),
                                  Data.GetKeyArriveTangentWeight(Handle1), Data.GetKeyTangentWeightMode(First), InTime);
}

}

float FCompressedRichCurve::Eval(float InTime, float InDefaultValue) const {
    return StaticEval(CompressionFormat, KeyTimeCompressionFormat, PreInfinityExtrap, PostInfinityExtrap, ConstantValueNumKeys.NumKeys,
                      CompressedKeys.GetData(), InTime, InDefaultValue);
}

float FCompressedRichCurve::StaticEval(ERichCurveCompressionFormat CompressionFormat, ERichCurveKeyTimeCompressionFormat KeyTimeCompressionFormat,
                                       ERichCurveExtrapolation PreInfinityExtrap, ERichCurveExtrapolation PostInfinityExtrap,
                                       int32 ConstantValueNumKeys, const uint8* CompressedKeys, float InTime, float InDefaultValue) {
    float ConstantValue;
    std::memcpy(&ConstantValue, &ConstantValueNumKeys, sizeof(float));
    if (CompressionFormat == RCCF_Empty) return ConstantValue == UE_MAX_FLT ? InDefaultValue : ConstantValue;
    if (CompressionFormat == RCCF_Constant) return ConstantValue;
    const int32 NumKeys = ConstantValueNumKeys;
    const bool bQuantized = KeyTimeCompressionFormat == RCKTCF_uint16;
    const int32 TimeAlignment = bQuantized ? sizeof(uint16) : sizeof(float);
    int32 KeyTimesOffset = 0;
    if (CompressionFormat == RCCF_Mixed) KeyTimesOffset = AlignOffset(NumKeys, TimeAlignment);
    else if (CompressionFormat == RCCF_Weighted) KeyTimesOffset = AlignOffset(2 * NumKeys, TimeAlignment);
    const FKeyTimes Times(CompressedKeys, KeyTimesOffset, NumKeys, bQuantized);
    const FKeyData Data{CompressedKeys, Times.KeyDataOffset, 0, CompressionFormat};
    return InterpEval(InTime, Times, Data, NumKeys, PreInfinityExtrap, PostInfinityExtrap);
}

}
