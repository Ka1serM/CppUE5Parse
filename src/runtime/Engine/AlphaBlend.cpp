#include "ue5parse/runtime/Engine/AlphaBlend.h"

#include <cmath>

#include "ue5parse/runtime/Core/Math/UnrealMathUtility.h"
#include "ue5parse/runtime/Engine/Curves/CurveFloat.h"

// UE's AlphaBlend.cpp (UE 5.3).

namespace ue5parse::runtime {

FAlphaBlend::FAlphaBlend(float NewBlendTime) : BlendTime(NewBlendTime) { Reset(); }

FAlphaBlend::FAlphaBlend(const FAlphaBlendArgs& InArgs) : CustomCurve(InArgs.CustomCurve), BlendTime(InArgs.BlendTime), BlendOption(InArgs.BlendOption) {
    Reset();
}

FAlphaBlendArgs::FAlphaBlendArgs(const FAlphaBlend& InAlphaBlend)
    : CustomCurve(InAlphaBlend.GetCustomCurve()), BlendTime(InAlphaBlend.GetBlendTime()), BlendOption(InAlphaBlend.GetBlendOption()) {}

FAlphaBlend::FAlphaBlend(const FAlphaBlend& Other, float NewBlendTime)
    : CustomCurve(Other.CustomCurve), BlendTime(NewBlendTime), BlendOption(Other.BlendOption), BeginValue(Other.BeginValue),
      DesiredValue(Other.DesiredValue) {
    Reset();
}

void FAlphaBlend::ResetBlendTime() {
    if (BlendTime <= 0.0f) {
        BlendTimeRemaining = 0.0f;
        SetAlpha(1.0f);
    } else {
        BlendTimeRemaining = BlendTime * FMath::Abs(1.0f - AlphaLerp);
    }
    bNeedsToResetBlendTime = false;
}

void FAlphaBlend::ResetAlpha() {
    if (BeginValue == DesiredValue) {
        SetAlpha(1.0f);
    } else {
        AlphaLerp = (BlendedValue - BeginValue) / (DesiredValue - BeginValue);
        SetAlpha(AlphaLerp);
    }
    bNeedsToResetAlpha = false;
}

void FAlphaBlend::Reset() {
    if (BlendTime <= 0.0f) {
        SetAlpha(1.0f);
        BlendTimeRemaining = 0.0f;
    } else {
        SetAlpha(0.0f);
        BlendTimeRemaining = BlendTime * FMath::Abs(1.0f - AlphaLerp);
    }
    bNeedsToResetCachedDesiredBlendedValue = true;
    bNeedsToResetAlpha = false;
    bNeedsToResetBlendTime = false;
}

float FAlphaBlend::Update(float InDeltaTime) {
    if (bNeedsToResetAlpha) ResetAlpha();
    if (bNeedsToResetBlendTime) ResetBlendTime();
    if (IsComplete()) return 0.0f;
    if (BlendTimeRemaining > InDeltaTime) {
        AlphaLerp += (1.0f - AlphaLerp) / BlendTimeRemaining * InDeltaTime;
        BlendTimeRemaining -= InDeltaTime;
        SetAlpha(AlphaLerp);
        return 0.0f;
    }
    const float Overshoot = InDeltaTime - BlendTimeRemaining;
    BlendTimeRemaining = 0.0f;
    SetAlpha(1.0f);
    return Overshoot;
}

float FAlphaBlend::AlphaToBlendOption(float InAlpha, EAlphaBlendOption InBlendOption, UCurveFloat* InCustomCurve) {
    switch (InBlendOption) {
        case EAlphaBlendOption::Sinusoidal: return FMath::Clamp((std::sin(InAlpha * float(UE_PI) - float(UE_HALF_PI)) + 1.0f) / 2.0f, 0.0f, 1.0f);
        case EAlphaBlendOption::Cubic: return FMath::Clamp(FMath::CubicInterp<float>(0.0f, 0.0f, 1.0f, 0.0f, InAlpha), 0.0f, 1.0f);
        case EAlphaBlendOption::QuadraticInOut: return FMath::Clamp(FMath::InterpEaseInOut<float>(0.0f, 1.0f, InAlpha, 2), 0.0f, 1.0f);
        case EAlphaBlendOption::CubicInOut: return FMath::Clamp(FMath::InterpEaseInOut<float>(0.0f, 1.0f, InAlpha, 3), 0.0f, 1.0f);
        case EAlphaBlendOption::HermiteCubic: return FMath::Clamp(float(FMath::SmoothStep(0.0f, 1.0f, InAlpha)), 0.0f, 1.0f);
        case EAlphaBlendOption::QuarticInOut: return FMath::Clamp(FMath::InterpEaseInOut<float>(0.0f, 1.0f, InAlpha, 4), 0.0f, 1.0f);
        case EAlphaBlendOption::QuinticInOut: return FMath::Clamp(FMath::InterpEaseInOut<float>(0.0f, 1.0f, InAlpha, 5), 0.0f, 1.0f);
        case EAlphaBlendOption::CircularIn: return FMath::Clamp(FMath::InterpCircularIn<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
        case EAlphaBlendOption::CircularOut: return FMath::Clamp(FMath::InterpCircularOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
        case EAlphaBlendOption::CircularInOut: return FMath::Clamp(FMath::InterpCircularInOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
        case EAlphaBlendOption::ExpIn: return FMath::Clamp(FMath::InterpExpoIn<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
        case EAlphaBlendOption::ExpOut: return FMath::Clamp(FMath::InterpExpoOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
        case EAlphaBlendOption::ExpInOut: return FMath::Clamp(FMath::InterpExpoInOut<float>(0.0f, 1.0f, InAlpha), 0.0f, 1.0f);
        case EAlphaBlendOption::Custom:
            if (InCustomCurve != nullptr) {
                float Min = 0, Max = 0;
                InCustomCurve->GetTimeRange(Min, Max);
                return FMath::Clamp(InCustomCurve->GetFloatValue(Min + (Max - Min) * InAlpha), 0.0f, 1.0f);
            }
            break;
        default: break;
    }
    return FMath::Clamp(InAlpha, 0.0f, 1.0f);
}

void FAlphaBlend::SetValueRange(float Begin, float Desired) {
    BeginValue = Begin;
    DesiredValue = Desired;
    bNeedsToResetAlpha = true;
    bNeedsToResetCachedDesiredBlendedValue = true;
}

void FAlphaBlend::SetDesiredValue(float InDesired) { SetValueRange(BlendedValue, InDesired); }

void FAlphaBlend::SetAlpha(float InAlpha) {
    AlphaLerp = FMath::Clamp(InAlpha, 0.0f, 1.0f);
    AlphaBlend = AlphaToBlendOption(AlphaLerp, BlendOption, CustomCurve);
    BlendedValue = BeginValue + (DesiredValue - BeginValue) * AlphaBlend;
}

void FAlphaBlend::SetBlendTime(float InBlendTime) {
    BlendTime = FMath::Max(InBlendTime, 0.0f);
    bNeedsToResetBlendTime = true;
}

void FAlphaBlend::SetBlendOption(EAlphaBlendOption InBlendOption) {
    BlendOption = InBlendOption;
    bNeedsToResetCachedDesiredBlendedValue = true;
}

void FAlphaBlend::SetCustomCurve(UCurveFloat* InCustomCurve) {
    CustomCurve = InCustomCurve;
    bNeedsToResetCachedDesiredBlendedValue = true;
}

bool FAlphaBlend::IsComplete() const {
    if (bNeedsToResetCachedDesiredBlendedValue) {
        CachedDesiredBlendedValue = BeginValue + (DesiredValue - BeginValue) * AlphaToBlendOption(1.0f, BlendOption, CustomCurve);
        bNeedsToResetCachedDesiredBlendedValue = false;
    }
    return CachedDesiredBlendedValue == BlendedValue;
}

}
