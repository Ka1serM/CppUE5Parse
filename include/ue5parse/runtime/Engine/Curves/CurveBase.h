#pragma once

#include "ue5parse/runtime/Engine/Curves/RichCurve.h"

#include "CurveBase.generated.h"

namespace ue5parse::runtime {

// UCurveBase: a curve asset of one or more rich curves (UE's Curves/CurveBase.h).
UCLASS(Abstract)
class UCurveBase : public UObject {
    GENERATED_BODY()
public:
    // The time range over all its curves.
    void GetTimeRange(float& MinTime, float& MaxTime) const;
    void GetValueRange(float& MinValue, float& MaxValue) const;
    virtual int32 GetNumCurves() const { return 0; }
    virtual const FRichCurve* GetCurve(int32 Index) const { return nullptr; }
};

}
