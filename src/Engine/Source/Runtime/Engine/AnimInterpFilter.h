#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Math/UnrealMathUtility.h"

namespace ue5parse::runtime {

enum EFilterInterpolationType : int32 {
    BSIT_Average,
    BSIT_Linear,
    BSIT_Cubic,
    BSIT_EaseInOut,
    BSIT_ExponentialDecay,
    BSIT_SpringDamper,
};

struct FFilterData {
    float Input = 0;
    float Time = 0;

    FFilterData() = default;
    FFilterData(float InInput, float InTime) : Input(InInput), Time(InTime) {}
    void EnsureTimeIsValid(float CurrentTime, float ValidationWindow) {
        if (Diff(CurrentTime) > ValidationWindow) Time = 0.f;
    }
    bool IsValid() const { return Time > 0.f; }
    float Diff(float InTime) const { return InTime - Time; }
    void SetInput(float InData, float InTime) {
        Input = InData;
        Time = InTime;
    }
};

struct FFIRFilterTimeBased {
    FFIRFilterTimeBased() = default;

    void Initialize(float InWindowDuration, EFilterInterpolationType InInterpolationType, float InDampingRatio, float InMinValue, float InMaxValue,
                    float InMaxSpeed, bool bInClamp) {
        InterpolationType = InInterpolationType;
        FilterData.Empty();
        CurrentStackIndex = 0;
        WindowDuration = InWindowDuration;
        SetParams(InDampingRatio, InMinValue, InMaxValue, InMaxSpeed, bInClamp);
        CurrentTime = 0.f;
        LastOutput = 0.f;
    }
    void SetParams(float InDampingRatio, float InMinValue, float InMaxValue, float InMaxSpeed, bool bInClamp) {
        DampingRatio = InDampingRatio;
        MinValue = InMinValue;
        MaxValue = InMaxValue;
        MaxSpeed = InMaxSpeed;
        bClamp = bInClamp;
    }
    float UpdateAndGetFilteredData(float Input, float DeltaTime);
    void WrapToValue(float Input, float Range);
    void SetToValue(float Input);
    bool IsValid() const { return WindowDuration > 0.f; }

    float LastOutput = 0;

private:
    float GetInterpolationCoefficient(const FFilterData& Data) const;
    float CalculateFilteredOutput() const;
    int32 GetSafeCurrentStackIndex();
    void RefreshValidFilters();

    EFilterInterpolationType InterpolationType = BSIT_Linear;
    int32 CurrentStackIndex = 0;
    float WindowDuration = 0;
    float DampingRatio = 1;
    float MinValue = 0;
    float MaxValue = 0;
    float MaxSpeed = 0;
    bool bClamp = false;
    float CurrentTime = 0;
    TArray<FFilterData> FilterData;
};

}
