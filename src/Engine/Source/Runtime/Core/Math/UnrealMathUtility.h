#pragma once

#include <cmath>
#include <cstdint>
#include <type_traits>

#include "Engine/Source/Runtime/Core/Containers/Array.h"

#define UE_REAL_TO_FLOAT(argument) static_cast<float>(argument)

namespace ue5parse::runtime {

#define UE_PI (3.1415926535897932)
#define UE_TWO_PI (6.28318530717958647)
#define UE_EULERS_NUMBER (2.71828182845904523536)
#define UE_HALF_PI (1.57079632679489661)
#define UE_INV_PI (0.31830988618)
#define UE_SMALL_NUMBER (1.e-8f)
#define UE_KINDA_SMALL_NUMBER (1.e-4f)
#define UE_BIG_NUMBER (3.4e+38f)
#define UE_DOUBLE_SMALL_NUMBER (1.e-8)
#define UE_DOUBLE_KINDA_SMALL_NUMBER (1.e-4)
#define UE_THRESH_POINT_ON_PLANE (0.10f)
#define UE_THRESH_NORMALS_ARE_PARALLEL (0.999845f)
#define UE_THRESH_QUAT_NORMALIZED (0.01f)
#define UE_DELTA (0.00001f)
#define UE_FLOAT_NORMAL_THRESH (0.0001f)
#define UE_MAX_FLT (3.402823466e+38f)
#define UE_MAX_int32 (0x7fffffff)
#define UE_MIN_int32 (-0x7fffffff - 1)
#define ZERO_ANIMWEIGHT_THRESH (0.00001f)

template <class T>
struct TVector;
template <class T>
struct TVector2;

struct FMath {
    static TVector2<double> GetAzimuthAndElevation(const TVector<double>& Direction, const TVector<double>& AxisX, const TVector<double>& AxisY,
                                                   const TVector<double>& AxisZ);
    static TVector<double> GetBaryCentric2D(const TVector2<double>& Point, const TVector2<double>& A, const TVector2<double>& B, const TVector2<double>& C);
    static TVector<double> ClosestPointOnSegment(const TVector<double>& Point, const TVector<double>& StartPoint, const TVector<double>& EndPoint);

    template <class T>
    static constexpr T Abs(T A) {
        return A < T(0) ? -A : A;
    }
    template <class T>
    static constexpr T Sign(T A) {
        return A > T(0) ? T(1) : (A < T(0) ? T(-1) : T(0));
    }
    template <class T>
    static constexpr T Max(T A, T B) {
        return A >= B ? A : B;
    }
    template <class T>
    static constexpr T Min(T A, T B) {
        return A <= B ? A : B;
    }
    template <class T>
    static constexpr T Max3(T A, T B, T C) {
        return Max(Max(A, B), C);
    }
    template <class T>
    static constexpr T Min3(T A, T B, T C) {
        return Min(Min(A, B), C);
    }
    template <class T>
    static constexpr T Square(T A) {
        return A * A;
    }
    template <class T>
    static constexpr T Cube(T A) {
        return A * A * A;
    }
    template <class T>
    static constexpr T Clamp(T X, T MinValue, T MaxValue) {
        return X < MinValue ? MinValue : (X < MaxValue ? X : MaxValue);
    }
    template <class T, class U>
    static constexpr T Lerp(const T& A, const T& B, const U& Alpha) {
        return static_cast<T>(A + Alpha * (B - A));
    }

    static double Sqrt(double Value) { return std::sqrt(Value); }
    static float Sqrt(float Value) { return std::sqrt(Value); }
    static double InvSqrt(double Value) { return 1.0 / std::sqrt(Value); }
    static float InvSqrt(float Value) { return 1.0f / std::sqrt(Value); }
    static double Sin(double Value) { return std::sin(Value); }
    static double Cos(double Value) { return std::cos(Value); }
    static double Tan(double Value) { return std::tan(Value); }
    static double Asin(double Value) { return std::asin(Clamp(Value, -1.0, 1.0)); }
    static double Acos(double Value) { return std::acos(Clamp(Value, -1.0, 1.0)); }
    static double Atan(double Value) { return std::atan(Value); }
    static double Atan2(double Y, double X) { return std::atan2(Y, X); }
    static float Atan2(float Y, float X) { return std::atan2(Y, X); }
    static double Exp(double Value) { return std::exp(Value); }
    static float Exp(float Value) { return std::exp(Value); }
    static double Pow(double A, double B) { return std::pow(A, B); }
    static float Pow(float A, float B) { return std::pow(A, B); }
    static double Fractional(double Value) { return Value - std::trunc(Value); }
    static double Frac(double Value) { return Value - std::floor(Value); }
    static double FloorToDouble(double Value) { return std::floor(Value); }
    static double TruncToDouble(double Value) { return std::trunc(Value); }
    static float TruncToFloat(float Value) { return std::trunc(Value); }
    static int32 FloorToInt(double Value) { return static_cast<int32>(std::floor(Value)); }
    static int32 CeilToInt(double Value) { return static_cast<int32>(std::ceil(Value)); }
    static int32 RoundToInt(double Value) { return static_cast<int32>(std::floor(Value + 0.5)); }
    static int32 TruncToInt(double Value) { return static_cast<int32>(Value); }
    static int64 TruncToInt64(double Value) { return static_cast<int64>(Value); }
    static double Fmod(double X, double Y);
    static float Fmod(float X, float Y);
    static void SinCos(double* ScalarSin, double* ScalarCos, double Value) {
        *ScalarSin = std::sin(Value);
        *ScalarCos = std::cos(Value);
    }
    static void SinCos(float* ScalarSin, float* ScalarCos, float Value) {
        *ScalarSin = std::sin(Value);
        *ScalarCos = std::cos(Value);
    }
    static double FastAsin(double Value);

    template <class T>
    static constexpr T RadiansToDegrees(T Value) {
        return Value * static_cast<T>(180.0 / UE_PI);
    }
    template <class T>
    static constexpr T DegreesToRadians(T Value) {
        return Value * static_cast<T>(UE_PI / 180.0);
    }
    static double UnwindRadians(double A) {
        while (A > UE_PI) A -= UE_TWO_PI;
        while (A < -UE_PI) A += UE_TWO_PI;
        return A;
    }
    static double ClampAngle(double AngleDegrees, double MinAngleDegrees, double MaxAngleDegrees);

    static bool IsNearlyEqual(double A, double B, double ErrorTolerance = UE_DOUBLE_SMALL_NUMBER) { return Abs(A - B) <= ErrorTolerance; }
    static bool IsNearlyEqual(float A, float B, float ErrorTolerance = UE_SMALL_NUMBER) { return Abs(A - B) <= ErrorTolerance; }
    static bool IsNearlyZero(double Value, double ErrorTolerance = UE_DOUBLE_SMALL_NUMBER) { return Abs(Value) <= ErrorTolerance; }
    static bool IsNearlyZero(float Value, float ErrorTolerance = UE_SMALL_NUMBER) { return Abs(Value) <= ErrorTolerance; }

    static double GetRangePct(double MinValue, double MaxValue, double Value) {
        double Divisor = MaxValue - MinValue;
        if (IsNearlyZero(Divisor)) return Value >= MaxValue ? 1.0 : 0.0;
        return (Value - MinValue) / Divisor;
    }
    static double GetMappedRangeValueClamped(double InMin, double InMax, double OutMin, double OutMax, double Value) {
        double ClampedPct = Clamp(GetRangePct(InMin, InMax, Value), 0.0, 1.0);
        return Lerp(OutMin, OutMax, ClampedPct);
    }
    static double GetMappedRangeValueUnclamped(double InMin, double InMax, double OutMin, double OutMax, double Value) {
        return Lerp(OutMin, OutMax, GetRangePct(InMin, InMax, Value));
    }
    static double SmoothStep(double A, double B, double X) {
        if (X < A) return 0.0;
        if (X >= B) return 1.0;
        double InterpFraction = (X - A) / (B - A);
        return InterpFraction * InterpFraction * (3.0 - 2.0 * InterpFraction);
    }

    template <class T>
    static T InterpEaseIn(const T& A, const T& B, double Alpha, double Exp) {
        double ModifiedAlpha = std::pow(Alpha, Exp);
        return Lerp(A, B, ModifiedAlpha);
    }
    template <class T>
    static T InterpEaseOut(const T& A, const T& B, double Alpha, double Exp) {
        double ModifiedAlpha = 1.0 - std::pow(1.0 - Alpha, Exp);
        return Lerp(A, B, ModifiedAlpha);
    }
    template <class T>
    static T CubicInterp(const T& P0, const T& T0, const T& P1, const T& T1, const T& A) {
        const T A2 = A * A;
        const T A3 = A2 * A;
        return (T(2) * A3 - T(3) * A2 + T(1)) * P0 + (A3 - T(2) * A2 + A) * T0 + (A3 - A2) * T1 + (T(-2) * A3 + T(3) * A2) * P1;
    }
    template <class T>
    static T InterpEaseInOut(const T& A, const T& B, double Alpha, double Exp) {
        return Lerp(A, B, Alpha < 0.5 ? InterpEaseIn(0.0, 1.0, Alpha * 2.0, Exp) * 0.5 : InterpEaseOut(0.0, 1.0, Alpha * 2.0 - 1.0, Exp) * 0.5 + 0.5);
    }
    template <class T>
    static T InterpSinIn(const T& A, const T& B, double Alpha) {
        double ModifiedAlpha = -1.0 * std::cos(Alpha * UE_HALF_PI) + 1.0;
        return Lerp(A, B, ModifiedAlpha);
    }
    template <class T>
    static T InterpSinOut(const T& A, const T& B, double Alpha) {
        double ModifiedAlpha = std::sin(Alpha * UE_HALF_PI);
        return Lerp(A, B, ModifiedAlpha);
    }
    template <class T>
    static T InterpSinInOut(const T& A, const T& B, double Alpha) {
        return Lerp(A, B, Alpha < 0.5 ? InterpSinIn(0.0, 1.0, Alpha * 2.0) * 0.5 : InterpSinOut(0.0, 1.0, Alpha * 2.0 - 1.0) * 0.5 + 0.5);
    }
    template <class T>
    static T InterpExpoIn(const T& A, const T& B, double Alpha) {
        double ModifiedAlpha = Alpha == 0.0 ? 0.0 : std::pow(2.0, 10.0 * (Alpha - 1.0));
        return Lerp(A, B, ModifiedAlpha);
    }
    template <class T>
    static T InterpExpoOut(const T& A, const T& B, double Alpha) {
        double ModifiedAlpha = Alpha == 1.0 ? 1.0 : -std::pow(2.0, -10.0 * Alpha) + 1.0;
        return Lerp(A, B, ModifiedAlpha);
    }
    template <class T>
    static T InterpExpoInOut(const T& A, const T& B, double Alpha) {
        return Lerp(A, B, Alpha < 0.5 ? InterpExpoIn(0.0, 1.0, Alpha * 2.0) * 0.5 : InterpExpoOut(0.0, 1.0, Alpha * 2.0 - 1.0) * 0.5 + 0.5);
    }
    template <class T>
    static T InterpCircularIn(const T& A, const T& B, double Alpha) {
        double ModifiedAlpha = -1.0 * (std::sqrt(1.0 - Alpha * Alpha) - 1.0);
        return Lerp(A, B, ModifiedAlpha);
    }
    template <class T>
    static T InterpCircularOut(const T& A, const T& B, double Alpha) {
        Alpha -= 1.0;
        double ModifiedAlpha = std::sqrt(1.0 - Alpha * Alpha);
        return Lerp(A, B, ModifiedAlpha);
    }
    template <class T>
    static T InterpCircularInOut(const T& A, const T& B, double Alpha) {
        return Lerp(A, B, Alpha < 0.5 ? InterpCircularIn(0.0, 1.0, Alpha * 2.0) * 0.5 : InterpCircularOut(0.0, 1.0, Alpha * 2.0 - 1.0) * 0.5 + 0.5);
    }
    template <class T>
    static T InterpStep(const T& A, const T& B, double Alpha, int32 Steps) {
        if (Steps <= 1 || Alpha <= 0) return A;
        if (Alpha >= 1) return B;
        const double StepsAsFloat = static_cast<double>(Steps);
        const double NumIntervals = StepsAsFloat - 1.0;
        double ModifiedAlpha = std::floor(Alpha * StepsAsFloat) / NumIntervals;
        return Lerp(A, B, ModifiedAlpha);
    }

    static double FInterpTo(double Current, double Target, double DeltaTime, double InterpSpeed) {
        if (InterpSpeed <= 0.0) return Target;
        const double Dist = Target - Current;
        if (Square(Dist) < UE_SMALL_NUMBER) return Target;
        const double DeltaMove = Dist * Clamp(DeltaTime * InterpSpeed, 0.0, 1.0);
        return Current + DeltaMove;
    }
    static double FInterpConstantTo(double Current, double Target, double DeltaTime, double InterpSpeed) {
        const double Dist = Target - Current;
        if (Square(Dist) < UE_SMALL_NUMBER) return Target;
        const double Step = InterpSpeed * DeltaTime;
        return Current + Clamp(Dist, -Step, Step);
    }

    static int32 Rand();
    static double FRand();
    static float SRand();
    static int32 RandRange(int32 Min, int32 Max) {
        const int32 Range = (Max - Min) + 1;
        return Min + RandHelper(Range);
    }
    static double FRandRange(double InMin, double InMax) { return InMin + (InMax - InMin) * FRand(); }
    static int32 RandHelper(int32 A) { return A > 0 ? Min(TruncToInt(FRand() * static_cast<double>(A)), A - 1) : 0; }
    static bool RandBool() { return RandRange(0, 1) == 1; }

    template <class T>
    static T DivideAndRoundUp(T Dividend, T Divisor) {
        return (Dividend + Divisor - 1) / Divisor;
    }
    template <class T>
    static T Wrap(T X, T Min, T Max) {
        T Size = Max - Min;
        if (Size == 0) return Max;
        T EndVal = X;
        while (EndVal < Min) EndVal += Size;
        while (EndVal > Max) EndVal -= Size;
        return EndVal;
    }
    template <class T>
    static constexpr T InvExpApprox(T X) {
        constexpr T A(1.00746054f);
        constexpr T B(0.45053901f);
        constexpr T C(0.25724632f);
        return 1 / (1 + A * X + B * X * X + C * X * X * X);
    }
    template <class T>
    static constexpr void ExponentialSmoothingApprox(T& InOutValue, const T& InTargetValue, const float InDeltaTime, const float InSmoothingTime) {
        if (InSmoothingTime <= UE_KINDA_SMALL_NUMBER) {
            InOutValue = InTargetValue;
            return;
        }
        const float Exp = InvExpApprox(InDeltaTime / InSmoothingTime);
        InOutValue = InTargetValue + (InOutValue - InTargetValue) * Exp;
    }
    template <class T>
    static constexpr void CriticallyDampedSmoothing(T& InOutValue, T& InOutValueRate, const T& InTargetValue, const T& InTargetValueRate,
                                                    const float InDeltaTime, const float InSmoothingTime) {
        if (InSmoothingTime > UE_KINDA_SMALL_NUMBER) {
            const T AdjustedTarget = InTargetValue + InTargetValueRate * InSmoothingTime;
            const float W = 2.0f / InSmoothingTime;
            const float Exp = InvExpApprox(W * InDeltaTime);
            T X0 = InOutValue - AdjustedTarget;
            X0 -= InTargetValueRate * InSmoothingTime;
            const T B = (InOutValueRate + X0 * W) * InDeltaTime;
            InOutValue = AdjustedTarget + (X0 + B) * Exp;
            InOutValueRate = (InOutValueRate - B * W) * Exp;
        } else if (InDeltaTime > 0.0f) {
            InOutValueRate = (InTargetValue - InOutValue) / InDeltaTime;
            InOutValue = InTargetValue;
        } else {
            InOutValue = InTargetValue;
            InOutValueRate = T(0);
        }
    }
    template <class T>
    static void SpringDamper(T& InOutValue, T& InOutValueRate, const T& InTargetValue, const T& InTargetValueRate, const float InDeltaTime,
                             const float InUndampedFrequency, const float InDampingRatio) {
        if (InDeltaTime <= 0.0f) return;
        const float W = InUndampedFrequency * static_cast<float>(UE_TWO_PI);
        if (W < UE_SMALL_NUMBER) {
            InOutValue += InOutValueRate * InDeltaTime;
            return;
        }
        if (InDampingRatio < UE_SMALL_NUMBER) {
            const T Err = InOutValue - InTargetValue;
            const T B = InOutValueRate / W;
            float S, C;
            SinCos(&S, &C, W * InDeltaTime);
            InOutValue = InTargetValue + Err * C + B * S;
            InOutValueRate = InOutValueRate * C - Err * (W * S);
            return;
        }
        const float SmoothingTime = 2.0f / W;
        const T AdjustedTarget = InTargetValue + InTargetValueRate * (InDampingRatio * SmoothingTime);
        const T Err = InOutValue - AdjustedTarget;
        if (InDampingRatio > 1.0f) {
            const float WD = W * Sqrt(Square(InDampingRatio) - 1.0f);
            const T C2 = -(InOutValueRate + (W * InDampingRatio - WD) * Err) / (2.0f * WD);
            const T C1 = Err - C2;
            const float A1 = WD - InDampingRatio * W;
            const float A2 = -(WD + InDampingRatio * W);
            const float E1 = InvExpApprox(-A1 * InDeltaTime);
            const float E2 = InvExpApprox(-A2 * InDeltaTime);
            InOutValue = AdjustedTarget + E1 * C1 + E2 * C2;
            InOutValueRate = E1 * C1 * A1 + E2 * C2 * A2;
        } else if (InDampingRatio < 1.0f) {
            const float WD = W * Sqrt(1.0f - Square(InDampingRatio));
            const T A = Err;
            const T B = (InOutValueRate + Err * (InDampingRatio * W)) / WD;
            float S, C;
            SinCos(&S, &C, WD * InDeltaTime);
            const float E = InvExpApprox(InDampingRatio * W * InDeltaTime);
            InOutValue = E * (A * C + B * S);
            InOutValueRate = -InOutValue * InDampingRatio * W;
            InOutValueRate += E * (B * (WD * C) - A * (WD * S));
            InOutValue += AdjustedTarget;
        } else {
            const T& C1 = Err;
            const T C2 = InOutValueRate + Err * W;
            const float E = InvExpApprox(W * InDeltaTime);
            InOutValue = AdjustedTarget + (C1 + C2 * InDeltaTime) * E;
            InOutValueRate = (C2 - C1 * W - C2 * (W * InDeltaTime)) * E;
        }
    }
    template <class T>
    static void SpringDamperSmoothing(T& InOutValue, T& InOutValueRate, const T& InTargetValue, const T& InTargetValueRate, const float InDeltaTime,
                                      const float InSmoothingTime, const float InDampingRatio) {
        if (InSmoothingTime < UE_SMALL_NUMBER) {
            if (InDeltaTime <= 0.0f) return;
            InOutValueRate = (InTargetValue - InOutValue) / InDeltaTime;
            InOutValue = InTargetValue;
            return;
        }
        SpringDamper(InOutValue, InOutValueRate, InTargetValue, InTargetValueRate, InDeltaTime, 1.0f / (static_cast<float>(UE_PI) * InSmoothingTime),
                     InDampingRatio);
    }
};

}
