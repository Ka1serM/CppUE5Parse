#include "ue5parse/runtime/Engine/Curves/RealCurve.h"

namespace ue5parse::runtime {

void FRealCurve::CycleTime(float MinTime, float MaxTime, float& InTime, int32& CycleCount) {
    const float InitTime = InTime;
    const float Duration = MaxTime - MinTime;
    if (InTime > MaxTime) {
        CycleCount = FMath::FloorToInt((MaxTime - InTime) / Duration);
        InTime = InTime + Duration * static_cast<float>(CycleCount);
    } else if (InTime < MinTime) {
        CycleCount = FMath::FloorToInt((InTime - MinTime) / Duration);
        InTime = InTime - Duration * static_cast<float>(CycleCount);
    }
    if (InTime == MaxTime && InitTime < MinTime) InTime = MinTime;
    if (InTime == MinTime && InitTime > MaxTime) InTime = MaxTime;
    CycleCount = FMath::Abs(CycleCount);
}

}
