#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

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

USTRUCT()
struct FAlphaBlendArgs {
    GENERATED_BODY()
    UPROPERTY()
    TObjectPtr<UCurveFloat> CustomCurve = nullptr;
    UPROPERTY()
    float BlendTime = 0.2f;
    UPROPERTY()
    EAlphaBlendOption BlendOption = EAlphaBlendOption::Linear;

    FAlphaBlendArgs() = default;
    explicit FAlphaBlendArgs(float InBlendTime) : BlendTime(InBlendTime) {}
    explicit FAlphaBlendArgs(const struct FAlphaBlend& InAlphaBlend);
};

USTRUCT()
struct FAlphaBlend {
    GENERATED_BODY()
public:
    UPROPERTY()
    TObjectPtr<UCurveFloat> CustomCurve = nullptr;
    UPROPERTY()
    float BlendTime = 0.2f;
    UPROPERTY()
    EAlphaBlendOption BlendOption = EAlphaBlendOption::Linear;

    explicit FAlphaBlend(float NewBlendTime = 0.2f);
    explicit FAlphaBlend(const FAlphaBlendArgs& InArgs);
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

    void Reset();
    float Update(float InDeltaTime);
    bool IsComplete() const;
    void SetBlendTime(float InBlendTime);
    void SetBlendOption(EAlphaBlendOption InBlendOption);
    void SetCustomCurve(UCurveFloat* InCustomCurve);
    void SetValueRange(float Begin, float Desired);
    void SetDesiredValue(float InDesired);
    void SetAlpha(float InAlpha);
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
