#include "ue5parse/runtime/Engine/Curves/SimpleCurve.h"

namespace ue5parse::runtime {

bool FSimpleCurveKey::Serialize(FArchive& Ar) {
    Ar << Time << Value;
    return true;
}

float FSimpleCurve::EvalForTwoKeys(const FSimpleCurveKey& Key1, const FSimpleCurveKey& Key2, float InTime) const {
    const float Diff = Key2.Time - Key1.Time;
    if (Diff > 0.f && InterpMode != RCIM_Constant) return FMath::Lerp(Key1.Value, Key2.Value, (InTime - Key1.Time) / Diff);
    return Key1.Value;
}

void FSimpleCurve::RemapTimeValue(float& InTime, float& CycleValueOffset) const {
    const int32 NumKeys = Keys.Num();
    if (NumKeys < 2) return;
    if (InTime <= Keys[0].Time) {
        if (PreInfinityExtrap != RCCE_Linear && PreInfinityExtrap != RCCE_Constant) {
            const float MinTime = Keys[0].Time;
            const float MaxTime = Keys[NumKeys - 1].Time;
            int32 CycleCount = 0;
            CycleTime(MinTime, MaxTime, InTime, CycleCount);
            if (PreInfinityExtrap == RCCE_CycleWithOffset) {
                CycleValueOffset = (Keys[0].Value - Keys[NumKeys - 1].Value) * static_cast<float>(CycleCount);
            } else if (PreInfinityExtrap == RCCE_Oscillate && CycleCount % 2 == 1) {
                InTime = MinTime + (MaxTime - InTime);
            }
        }
    } else if (InTime >= Keys[NumKeys - 1].Time) {
        if (PostInfinityExtrap != RCCE_Linear && PostInfinityExtrap != RCCE_Constant) {
            const float MinTime = Keys[0].Time;
            const float MaxTime = Keys[NumKeys - 1].Time;
            int32 CycleCount = 0;
            CycleTime(MinTime, MaxTime, InTime, CycleCount);
            if (PostInfinityExtrap == RCCE_CycleWithOffset) {
                CycleValueOffset = (Keys[NumKeys - 1].Value - Keys[0].Value) * static_cast<float>(CycleCount);
            } else if (PostInfinityExtrap == RCCE_Oscillate && CycleCount % 2 == 1) {
                InTime = MinTime + (MaxTime - InTime);
            }
        }
    }
}

float FSimpleCurve::Eval(float InTime, float InDefaultValue) const {
    float CycleValueOffset = 0;
    RemapTimeValue(InTime, CycleValueOffset);
    const int32 NumKeys = Keys.Num();
    float InterpVal = DefaultValue == UE_MAX_FLT ? InDefaultValue : DefaultValue;
    if (NumKeys == 0) {
        // The default value.
    } else if (NumKeys < 2 || InTime <= Keys[0].Time) {
        if (PreInfinityExtrap == RCCE_Linear && NumKeys > 1) {
            const float DT = Keys[1].Time - Keys[0].Time;
            if (FMath::IsNearlyZero(DT)) {
                InterpVal = Keys[0].Value;
            } else {
                InterpVal = (Keys[1].Value - Keys[0].Value) / DT * (InTime - Keys[0].Time) + Keys[0].Value;
            }
        } else {
            InterpVal = Keys[0].Value;
        }
    } else if (InTime < Keys[NumKeys - 1].Time) {
        int32 First = 1;
        int32 Count = NumKeys - 1 - First;
        while (Count > 0) {
            const int32 Step = Count / 2;
            const int32 Middle = First + Step;
            if (InTime >= Keys[Middle].Time) {
                First = Middle + 1;
                Count -= Step + 1;
            } else {
                Count = Step;
            }
        }
        InterpVal = EvalForTwoKeys(Keys[First - 1], Keys[First], InTime);
    } else {
        if (PostInfinityExtrap == RCCE_Linear) {
            const float DT = Keys[NumKeys - 2].Time - Keys[NumKeys - 1].Time;
            if (FMath::IsNearlyZero(DT)) {
                InterpVal = Keys[NumKeys - 1].Value;
            } else {
                InterpVal = (Keys[NumKeys - 2].Value - Keys[NumKeys - 1].Value) / DT * (InTime - Keys[NumKeys - 1].Time) + Keys[NumKeys - 1].Value;
            }
        } else {
            InterpVal = Keys[NumKeys - 1].Value;
        }
    }
    return InterpVal + CycleValueOffset;
}

}
