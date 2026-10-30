#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/Object.h"
#include "ue5parse/runtime/Engine/AlphaBlend.h"

#include "InputScaleBias.generated.h"

// The scale, bias, range mapping, clamp and interpolation of a node's input value (UE's Animation/InputScaleBias.h).
namespace ue5parse::runtime {

// How a node reads its alpha: a float, a bool that blends in and out, or a curve of the pose.
enum class EAnimAlphaInputType : uint8 { Float, Bool, Curve };

USTRUCT()
struct FInputRange {
    GENERATED_BODY()
public:
    UPROPERTY()
    float Min = 0;
    UPROPERTY()
    float Max = 1;
};

USTRUCT()
struct FInputScaleBias {
    GENERATED_BODY()
public:
    UPROPERTY()
    float Scale = 1;
    UPROPERTY()
    float Bias = 0;

    // Value * Scale + Bias, clamped to [0, 1].
    float ApplyTo(float Value) const;
};

USTRUCT()
struct FInputScaleBiasClampConstants {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bMapRange = false;
    UPROPERTY()
    bool bClampResult = false;
    UPROPERTY()
    bool bInterpResult = false;
    UPROPERTY()
    FInputRange InRange;
    UPROPERTY()
    FInputRange OutRange;
    UPROPERTY()
    float Scale = 1;
    UPROPERTY()
    float Bias = 0;
    UPROPERTY()
    float ClampMin = 0;
    UPROPERTY()
    float ClampMax = 1;
    UPROPERTY()
    float InterpSpeedIncreasing = 10;
    UPROPERTY()
    float InterpSpeedDecreasing = 10;
};

// The interpolation state that goes with a FInputScaleBiasClampConstants.
USTRUCT()
struct FInputScaleBiasClampState {
    GENERATED_BODY()
public:
    void Reinitialize() { bInitialized = false; }
    float ApplyTo(const FInputScaleBiasClampConstants& InConstants, float InValue, float InDeltaTime);
    // Without interpolation, and without changing the state.
    float ApplyTo(const FInputScaleBiasClampConstants& InConstants, float InValue) const;

private:
    float InterpolatedResult = 0;
    bool bInitialized = false;
};

// The constants and the state in one struct.
USTRUCT()
struct FInputScaleBiasClamp {
    GENERATED_BODY()
public:
    UPROPERTY()
    bool bMapRange = false;
    UPROPERTY()
    bool bClampResult = false;
    UPROPERTY()
    bool bInterpResult = false;
    UPROPERTY()
    FInputRange InRange;
    UPROPERTY()
    FInputRange OutRange;
    UPROPERTY()
    float Scale = 1;
    UPROPERTY()
    float Bias = 0;
    UPROPERTY()
    float ClampMin = 0;
    UPROPERTY()
    float ClampMax = 1;
    UPROPERTY()
    float InterpSpeedIncreasing = 10;
    UPROPERTY()
    float InterpSpeedDecreasing = 10;

    void Reinitialize() { bInitialized = false; }
    float ApplyTo(float Value, float InDeltaTime);

private:
    float InterpolatedResult = 0;
    bool bInitialized = false;
};

// A bool input that blends to 1 when it becomes true and to 0 when it becomes false. The first value is taken at once.
USTRUCT()
struct FInputAlphaBoolBlend {
    GENERATED_BODY()
public:
    UPROPERTY()
    float BlendInTime = 0;
    UPROPERTY()
    float BlendOutTime = 0;
    UPROPERTY()
    EAlphaBlendOption BlendOption = EAlphaBlendOption::Linear;
    UPROPERTY()
    bool bInitialized = false;
    UPROPERTY()
    UCurveFloat* CustomCurve = nullptr;
    UPROPERTY()
    FAlphaBlend AlphaBlend;

    void Reinitialize() { bInitialized = false; }
    float ApplyTo(bool bEnabled, float InDeltaTime);
};

}
