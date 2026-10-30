#include "ue5parse/runtime/Engine/Animation/InputScaleBias.h"

#include "ue5parse/runtime/Core/Math/UnrealMathUtility.h"

namespace ue5parse::runtime {

namespace {

float MapScaleBiasClamp(bool bMapRange, const FInputRange& InRange, const FInputRange& OutRange, float Scale, float Bias, bool bClampResult,
                        float ClampMin, float ClampMax, float Value) {
    float Result = Value;
    if (bMapRange) Result = float(FMath::GetMappedRangeValueUnclamped(InRange.Min, InRange.Max, OutRange.Min, OutRange.Max, Result));
    Result = Result * Scale + Bias;
    if (bClampResult) Result = FMath::Clamp(Result, ClampMin, ClampMax);
    return Result;
}

// Moves the interpolated result toward the target at the speed of its direction; the first value is taken as is.
float Interpolate(float Target, float SpeedIncreasing, float SpeedDecreasing, float DeltaTime, float& InterpolatedResult, bool bInitialized) {
    float Result = Target;
    if (bInitialized) {
        const float InterpSpeed = Result >= InterpolatedResult ? SpeedIncreasing : SpeedDecreasing;
        Result = float(FMath::FInterpTo(InterpolatedResult, Result, DeltaTime, InterpSpeed));
    }
    InterpolatedResult = Result;
    return Result;
}

}

float FInputScaleBias::ApplyTo(float Value) const { return FMath::Clamp(Value * Scale + Bias, 0.0f, 1.0f); }

float FInputScaleBiasClampState::ApplyTo(const FInputScaleBiasClampConstants& InConstants, float InValue, float InDeltaTime) {
    float Result = MapScaleBiasClamp(InConstants.bMapRange, InConstants.InRange, InConstants.OutRange, InConstants.Scale, InConstants.Bias,
                                     InConstants.bClampResult, InConstants.ClampMin, InConstants.ClampMax, InValue);
    if (InConstants.bInterpResult)
        Result = Interpolate(Result, InConstants.InterpSpeedIncreasing, InConstants.InterpSpeedDecreasing, InDeltaTime, InterpolatedResult, bInitialized);
    bInitialized = true;
    return Result;
}

float FInputScaleBiasClampState::ApplyTo(const FInputScaleBiasClampConstants& InConstants, float InValue) const {
    return MapScaleBiasClamp(InConstants.bMapRange, InConstants.InRange, InConstants.OutRange, InConstants.Scale, InConstants.Bias,
                             InConstants.bClampResult, InConstants.ClampMin, InConstants.ClampMax, InValue);
}

float FInputScaleBiasClamp::ApplyTo(float Value, float InDeltaTime) {
    float Result = MapScaleBiasClamp(bMapRange, InRange, OutRange, Scale, Bias, bClampResult, ClampMin, ClampMax, Value);
    if (bInterpResult) Result = Interpolate(Result, InterpSpeedIncreasing, InterpSpeedDecreasing, InDeltaTime, InterpolatedResult, bInitialized);
    bInitialized = true;
    return Result;
}

float FInputAlphaBoolBlend::ApplyTo(bool bEnabled, float InDeltaTime) {
    const float TargetValue = bEnabled ? 1.0f : 0.0f;
    if (!bInitialized) {
        if (CustomCurve != AlphaBlend.GetCustomCurve()) AlphaBlend.SetCustomCurve(CustomCurve);
        if (BlendOption != AlphaBlend.GetBlendOption()) AlphaBlend.SetBlendOption(BlendOption);
        AlphaBlend.SetDesiredValue(TargetValue);
        AlphaBlend.SetBlendTime(0.0f);
        AlphaBlend.Reset();
        bInitialized = true;
    } else if (AlphaBlend.GetDesiredValue() != TargetValue) {
        AlphaBlend.SetDesiredValue(TargetValue);
        AlphaBlend.SetBlendTime(bEnabled ? BlendInTime : BlendOutTime);
    }
    AlphaBlend.Update(InDeltaTime);
    return AlphaBlend.GetBlendedValue();
}

}
