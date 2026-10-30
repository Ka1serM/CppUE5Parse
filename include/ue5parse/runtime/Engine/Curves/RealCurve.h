#pragma once

#include "ue5parse/runtime/Engine/Curves/IndexedCurve.h"

#include "RealCurve.generated.h"

namespace ue5parse::runtime {

enum ERichCurveInterpMode : int32 { RCIM_Linear, RCIM_Constant, RCIM_Cubic, RCIM_None };
enum ERichCurveTangentMode : int32 { RCTM_Auto, RCTM_User, RCTM_Break, RCTM_None, RCTM_SmartAuto };
enum ERichCurveTangentWeightMode : int32 { RCTWM_WeightedNone, RCTWM_WeightedArrive, RCTWM_WeightedLeave, RCTWM_WeightedBoth };
enum ERichCurveExtrapolation : int32 { RCCE_Cycle, RCCE_CycleWithOffset, RCCE_Oscillate, RCCE_Linear, RCCE_Constant, RCCE_None };

// FRealCurve: a curve of float values with a default and infinity extrapolation (UE's Curves/RealCurve.h).
USTRUCT()
struct FRealCurve : public FIndexedCurve {
    GENERATED_BODY()
    UPROPERTY()
    float DefaultValue = UE_MAX_FLT;
    UPROPERTY()
    TEnumAsByte<ERichCurveExtrapolation> PreInfinityExtrap = RCCE_Constant;
    UPROPERTY()
    TEnumAsByte<ERichCurveExtrapolation> PostInfinityExtrap = RCCE_Constant;

    void SetDefaultValue(float InDefaultValue) { DefaultValue = InDefaultValue; }
    float GetDefaultValue() const { return DefaultValue; }
    void ClearDefaultValue() { DefaultValue = UE_MAX_FLT; }
    virtual float Eval(float InTime, float InDefaultValue = 0.0f) const { return InDefaultValue; }

protected:
    // Wraps a time outside [MinTime, MaxTime] into it, counting the cycles (UE's FRealCurve::CycleTime).
    static void CycleTime(float MinTime, float MaxTime, float& InTime, int32& CycleCount);
};

}
