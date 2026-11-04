#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Object.h"
#include "Engine/Source/Runtime/Engine/AlphaBlend.h"

#include "InputScaleBias.generated.h"

namespace ue5parse::runtime {

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

USTRUCT()
struct FInputScaleBiasClampState {
    GENERATED_BODY()
public:
    void Reinitialize() { bInitialized = false; }
    float ApplyTo(const FInputScaleBiasClampConstants& InConstants, float InValue, float InDeltaTime);
    float ApplyTo(const FInputScaleBiasClampConstants& InConstants, float InValue) const;

private:
    float InterpolatedResult = 0;
    bool bInitialized = false;
};

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
    TObjectPtr<UCurveFloat> CustomCurve = nullptr;
    UPROPERTY()
    FAlphaBlend AlphaBlend;

    void Reinitialize() { bInitialized = false; }
    float ApplyTo(bool bEnabled, float InDeltaTime);
};

}
