#pragma once

#include <cmath>

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Math/InterpCurvePoint.h"
#include "Engine/Source/Runtime/Core/Math/UnrealMathUtility.h"

namespace ue5parse::runtime {

template <class T>
class FInterpCurve {
public:
    TArray<FInterpCurvePoint<T>> Points;
    bool bIsLooped = false;
    float LoopKeyOffset = 0.0f;

    int32 GetPointIndexForInputValue(float InValue) const {
        const int32 LastPoint = Points.Num() - 1;
        if (InValue < Points[0].InVal)
            return -1;
        if (InValue >= Points[LastPoint].InVal)
            return LastPoint;
        int32 MinIndex = 0;
        int32 MaxIndex = Points.Num();
        while (MaxIndex - MinIndex > 1) {
            const int32 MidIndex = (MinIndex + MaxIndex) / 2;
            if (Points[MidIndex].InVal <= InValue)
                MinIndex = MidIndex;
            else
                MaxIndex = MidIndex;
        }
        return MinIndex;
    }

    T Eval(float InVal, const T& Default) const {
        const int32 LastPoint = Points.Num() - 1;
        if (Points.Num() == 0 || std::isnan(InVal))
            return Default;
        const int32 Index = GetPointIndexForInputValue(InVal);
        if (Index == -1)
            return Points[0].OutVal;
        if (Index == LastPoint && !bIsLooped)
            return Points[LastPoint].OutVal;
        if (Index == LastPoint && InVal >= Points[LastPoint].InVal + LoopKeyOffset)
            return Points[0].OutVal;
        const bool bLoopSegment = bIsLooped && Index == LastPoint;
        const FInterpCurvePoint<T>& PrevPoint = Points[Index];
        const FInterpCurvePoint<T>& NextPoint = Points[bLoopSegment ? 0 : Index + 1];
        const float Diff = bLoopSegment ? LoopKeyOffset : NextPoint.InVal - PrevPoint.InVal;
        if (Diff <= 0.0f || PrevPoint.InterpMode == CIM_Constant)
            return PrevPoint.OutVal;
        const float Alpha = (InVal - PrevPoint.InVal) / Diff;
        if (PrevPoint.InterpMode == CIM_Linear)
            return FMath::Lerp(PrevPoint.OutVal, NextPoint.OutVal, Alpha);
        return FMath::CubicInterp(PrevPoint.OutVal, PrevPoint.LeaveTangent * Diff, NextPoint.OutVal, NextPoint.ArriveTangent * Diff, Alpha);
    }

    void CalcBounds(T& OutMin, T& OutMax, const T& Default) const {
        if (Points.Num() == 0) {
            OutMin = Default;
            OutMax = Default;
            return;
        }
        OutMin = Points[0].OutVal;
        OutMax = Points[0].OutVal;
        if (Points.Num() == 1)
            return;
        const int32 NumSegments = bIsLooped ? Points.Num() : Points.Num() - 1;
        for (int32 Index = 0; Index < NumSegments; Index++)
            CurveFindIntervalBounds(Points[Index], Points[Index == Points.Num() - 1 ? 0 : Index + 1], OutMin, OutMax);
    }
};

struct FInterpCurveFloat : FInterpCurve<float> {};
struct FInterpCurveVector : FInterpCurve<FVector> {};

}
