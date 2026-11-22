#pragma once

#include "Engine/Source/Runtime/Core/Containers/EnumAsByte.h"
#include "Engine/Source/Runtime/Core/Math/Quat.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"

namespace ue5parse::runtime {

enum EInterpCurveMode : int32 { CIM_Linear, CIM_CurveAuto, CIM_Constant, CIM_CurveUser, CIM_CurveBreak, CIM_CurveAutoClamped, CIM_Unknown };

template <class T>
class FInterpCurvePoint {
public:
    float InVal = 0.0f;
    T OutVal{};
    T ArriveTangent{};
    T LeaveTangent{};
    TEnumAsByte<EInterpCurveMode> InterpMode = CIM_Linear;

    bool IsCurveKey() const {
        return InterpMode == CIM_CurveAuto || InterpMode == CIM_CurveAutoClamped || InterpMode == CIM_CurveUser || InterpMode == CIM_CurveBreak;
    }
};

using FInterpCurvePointFloat = FInterpCurvePoint<float>;
using FInterpCurvePointVector = FInterpCurvePoint<FVector>;
using FInterpCurvePointQuat = FInterpCurvePoint<FQuat>;

void CurveFindIntervalBounds(const FInterpCurvePoint<float>& Start, const FInterpCurvePoint<float>& End, float& CurrentMin, float& CurrentMax);
void CurveFindIntervalBounds(const FInterpCurvePoint<FVector>& Start, const FInterpCurvePoint<FVector>& End, FVector& CurrentMin, FVector& CurrentMax);

}
