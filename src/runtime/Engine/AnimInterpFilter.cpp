#include "ue5parse/runtime/Engine/AnimInterpFilter.h"

// UE's AnimInterpFilter.cpp (UE 5.3).

namespace ue5parse::runtime {

int32 FFIRFilterTimeBased::GetSafeCurrentStackIndex() {
    if (!FilterData[CurrentStackIndex].IsValid()) return CurrentStackIndex;
    for (int32 I = 0; I != FilterData.Num(); ++I) {
        int32 NewIndex = CurrentStackIndex + I;
        if (NewIndex >= FilterData.Num()) NewIndex -= FilterData.Num();
        if (!FilterData[NewIndex].IsValid()) return NewIndex;
    }
    // Every entry is in the window: grow the stack.
    const int32 NewIndex = FilterData.Num();
    FilterData.AddDefaulted(5);
    return NewIndex;
}

void FFIRFilterTimeBased::RefreshValidFilters() {
    if (FilterData.IsEmpty()) {
        FilterData.AddDefaulted(10);
        CurrentStackIndex = 0;
        return;
    }
    for (FFilterData& Data : FilterData) Data.EnsureTimeIsValid(CurrentTime, WindowDuration);
}

void FFIRFilterTimeBased::WrapToValue(float Input, float Range) {
    if (Range <= 0.0f) return;
    const float HalfRange = Range / 2.0f;
    switch (InterpolationType) {
        case BSIT_ExponentialDecay:
        case BSIT_SpringDamper:
            if (FilterData.Num() != 0) FilterData[0].Input = FMath::Wrap(FilterData[0].Input, Input - HalfRange, Input + HalfRange);
            break;
        default: {
            if (!IsValid()) break;
            const float NewLastOutput = FMath::Wrap(LastOutput, Input - HalfRange, Input + HalfRange);
            const float Delta = NewLastOutput - LastOutput;
            if (Delta == 0.0f) break;
            LastOutput = NewLastOutput;
            for (FFilterData& Data : FilterData) Data.Input += Delta;
            break;
        }
    }
}

void FFIRFilterTimeBased::SetToValue(float Value) {
    LastOutput = Value;
    if (FilterData.Num() > 0) FilterData[0].Input = Value;
}

float FFIRFilterTimeBased::UpdateAndGetFilteredData(float Input, float DeltaTime) {
    if (WindowDuration <= 0.0f) {
        LastOutput = Input;
        return Input;
    }
    if (DeltaTime <= UE_KINDA_SMALL_NUMBER) return LastOutput;
    float Result;
    CurrentTime += DeltaTime;
    switch (InterpolationType) {
        case BSIT_ExponentialDecay: {
            if (FilterData.Num() != 1) {
                FilterData.Empty(1);
                FilterData.Add(FFilterData(Input, 0.0f));
            }
            const float OrigValue = FilterData[0].Input;
            FMath::ExponentialSmoothingApprox(FilterData[0].Input, Input, DeltaTime, WindowDuration / static_cast<float>(UE_EULERS_NUMBER));
            if (MaxSpeed > 0.0f) FilterData[0].Input = FMath::Clamp(FilterData[0].Input, OrigValue - MaxSpeed * DeltaTime, OrigValue + MaxSpeed * DeltaTime);
            Result = FilterData[0].Input;
            break;
        }
        case BSIT_SpringDamper: {
            // Entry 0 holds the value, entry 1 its rate.
            if (FilterData.Num() != 2) {
                FilterData.Empty(2);
                FilterData.Add(FFilterData(Input, 0.0f));
                FilterData.Add(FFilterData(0.0f, 0.0f));
            }
            const float OrigValue = FilterData[0].Input;
            FMath::SpringDamperSmoothing(FilterData[0].Input, FilterData[1].Input, Input, 0.0f, DeltaTime,
                                         WindowDuration / static_cast<float>(UE_EULERS_NUMBER), DampingRatio);
            if (MaxSpeed > 0.0f) {
                FilterData[0].Input = FMath::Clamp(FilterData[0].Input, OrigValue - MaxSpeed * DeltaTime, OrigValue + MaxSpeed * DeltaTime);
                FilterData[1].Input = FMath::Clamp(FilterData[1].Input, -MaxSpeed, MaxSpeed);
            }
            if (bClamp) {
                if (FilterData[0].Input > MaxValue) {
                    FilterData[0].Input = MaxValue;
                    if (FilterData[1].Input > 0.0f) FilterData[1].Input = 0.0f;
                }
                if (FilterData[0].Input < MinValue) {
                    FilterData[0].Input = MinValue;
                    if (FilterData[1].Input < 0.0f) FilterData[1].Input = 0.0f;
                }
            }
            Result = FilterData[0].Input;
            break;
        }
        default: {
            if (!IsValid()) {
                Result = Input;
                break;
            }
            RefreshValidFilters();
            CurrentStackIndex = GetSafeCurrentStackIndex();
            FilterData[CurrentStackIndex].SetInput(Input, CurrentTime);
            Result = CalculateFilteredOutput();
            if (++CurrentStackIndex > FilterData.Num() - 1) CurrentStackIndex = 0;
            break;
        }
    }
    LastOutput = Result;
    return Result;
}

float FFIRFilterTimeBased::GetInterpolationCoefficient(const FFilterData& Data) const {
    if (!Data.IsValid()) return 0.f;
    const float Diff = Data.Diff(CurrentTime);
    if (Diff > WindowDuration) return 0.f;
    switch (InterpolationType) {
        case BSIT_Average: return 1.f;
        case BSIT_Linear: return 1.f - Diff / WindowDuration;
        case BSIT_Cubic: return 1.f - FMath::Cube(Diff / WindowDuration);
        case BSIT_EaseInOut: return 1.0f - 4.0f * FMath::Square(Diff / WindowDuration - 0.5f);
        default: return 0.f;
    }
}

float FFIRFilterTimeBased::CalculateFilteredOutput() const {
    float SumCoefficient = 0.f;
    float SumInputs = 0.f;
    for (const FFilterData& Data : FilterData) {
        const float Coefficient = GetInterpolationCoefficient(Data);
        if (Coefficient <= 0.f) continue;
        SumCoefficient += Coefficient;
        SumInputs += Coefficient * Data.Input;
    }
    return SumCoefficient > 0.f ? SumInputs / SumCoefficient : 0.f;
}

}
