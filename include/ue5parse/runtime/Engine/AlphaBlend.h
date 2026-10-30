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

// FAlphaBlend: a blend over time with an easing option (UE's AlphaBlend.h). Only the serialized members.
USTRUCT()
struct FAlphaBlend {
    GENERATED_BODY()
    UPROPERTY()
    UCurveFloat* CustomCurve = nullptr;
    UPROPERTY()
    float BlendTime = 0.2f;
    UPROPERTY()
    EAlphaBlendOption BlendOption = EAlphaBlendOption::Linear;

    FAlphaBlend() = default;
    explicit FAlphaBlend(float NewBlendTime) : BlendTime(NewBlendTime) {}
    float GetBlendTime() const { return BlendTime; }
    EAlphaBlendOption GetBlendOption() const { return BlendOption; }
    UCurveFloat* GetCustomCurve() const { return CustomCurve; }
};

}
