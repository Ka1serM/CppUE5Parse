#pragma once

#include "Engine/Source/Runtime/Engine/Curves/RichCurve.h"

#include "CurveBase.generated.h"

namespace ue5parse::runtime {

UCLASS(Abstract)
class UCurveBase : public UObject {
    GENERATED_BODY()
public:
    void GetTimeRange(float& MinTime, float& MaxTime) const;
    void GetValueRange(float& MinValue, float& MaxValue) const;
    virtual int32 GetNumCurves() const { return 0; }
    virtual const FRichCurve* GetCurve(int32 Index) const { return nullptr; }
};

}
