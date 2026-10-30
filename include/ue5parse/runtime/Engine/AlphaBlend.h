#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/PropertyAccess.h"

#include "AlphaBlend.generated.h"

namespace ue5parse::runtime {

class UCurveFloat;

enum class EAlphaBlendOption : uint8 {
    Linear,
    Cubic,
    HermiteCubic,
    Sinusoidal,
    QuadraticInOut,
    CubicInOut,
    QuarticInOut,
    QuinticInOut,
    CircularIn,
    CircularOut,
    CircularInOut,
    ExpIn,
    ExpOut,
    ExpInOut,
    Custom,
};

// FAlphaBlendArgs: the settings of a blend, as Blueprint passes them (UE's AlphaBlend.h).
USTRUCT()
struct FAlphaBlendArgs {
    GENERATED_BODY()
    UPROPERTY()
    UCurveFloat* CustomCurve = nullptr;
    UPROPERTY()
    float BlendTime = 0.2f;
    UPROPERTY()
    EAlphaBlendOption BlendOption = EAlphaBlendOption::Linear;

    FAlphaBlendArgs() = default;
    explicit FAlphaBlendArgs(float InBlendTime) : BlendTime(InBlendTime) {}
    explicit FAlphaBlendArgs(const struct FAlphaBlend& InAlphaBlend);
};

// FAlphaBlend: a value that blends from a begin value to a desired value over the blend time, with an easing option
// (UE's AlphaBlend.h). The serialized members are the settings; the others are the blend's state.
USTRUCT()
struct FAlphaBlend {
    GENERATED_BODY()
public:
    UPROPERTY()
    UCurveFloat* CustomCurve = nullptr;
    UPROPERTY()
    float BlendTime = 0.2f;
    UPROPERTY()
    EAlphaBlendOption BlendOption = EAlphaBlendOption::Linear;

    explicit FAlphaBlend(float NewBlendTime = 0.2f);
    explicit FAlphaBlend(const FAlphaBlendArgs& InArgs);
    // The settings of Other with another blend time.
    FAlphaBlend(const FAlphaBlend& Other, float NewBlendTime);
    FAlphaBlend(const FAlphaBlend&) = default;
    FAlphaBlend& operator=(const FAlphaBlend&) = default;

    float GetBlendTime() const { return BlendTime; }
    EAlphaBlendOption GetBlendOption() const { return BlendOption; }
    UCurveFloat* GetCustomCurve() const { return CustomCurve; }
    float GetBlendTimeRemaining() const { return BlendTimeRemaining; }
    float GetAlpha() const { return AlphaLerp; }
    float GetBlendedValue() const { return BlendedValue; }
    float GetDesiredValue() const { return DesiredValue; }

    // Restarts the blend from the begin value.
    void Reset();
    // Advances the blend; returns the time past its end.
    float Update(float InDeltaTime);
    bool IsComplete() const;
    void SetBlendTime(float InBlendTime);
    void SetBlendOption(EAlphaBlendOption InBlendOption);
    void SetCustomCurve(UCurveFloat* InCustomCurve);
    void SetValueRange(float Begin, float Desired);
    // Blends from the current value to InDesired.
    void SetDesiredValue(float InDesired);
    void SetAlpha(float InAlpha);
    // The eased alpha of a linear alpha.
    static float AlphaToBlendOption(float InAlpha, EAlphaBlendOption InBlendOption, UCurveFloat* InCustomCurve = nullptr);

private:
    void ResetBlendTime();
    void ResetAlpha();

    float AlphaLerp = 0;
    float AlphaBlend = 0;
    float BeginValue = 0;
    float DesiredValue = 1;
    float BlendedValue = 0;
    float BlendTimeRemaining = 0;
    mutable float CachedDesiredBlendedValue = 0;
    bool bNeedsToResetAlpha = false;
    bool bNeedsToResetBlendTime = false;
    mutable bool bNeedsToResetCachedDesiredBlendedValue = false;
};

}
