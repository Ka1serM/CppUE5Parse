#pragma once

#include "ue5parse/runtime/Engine/Curves/CurveBase.h"

#include "CurveFloat.generated.h"

namespace ue5parse::runtime {

class UCurveFloat;

// FRuntimeFloatCurve: a curve of its own, or an external curve asset that replaces it.
USTRUCT()
struct FRuntimeFloatCurve {
    GENERATED_BODY()
    UPROPERTY()
    FRichCurve EditorCurveData;
    UPROPERTY()
    UCurveFloat* ExternalCurve = nullptr;

    const FRichCurve* GetRichCurveConst() const;
    FRichCurve* GetRichCurve();
};

UCLASS()
class UCurveFloat : public UCurveBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FRichCurve FloatCurve;
    UPROPERTY()
    bool bIsEventCurve = false;

    UFUNCTION(BlueprintCallable)
    float GetFloatValue(float InTime) const { return FloatCurve.Eval(InTime); }
    int32 GetNumCurves() const override { return 1; }
    const FRichCurve* GetCurve(int32 Index) const override { return Index == 0 ? &FloatCurve : nullptr; }
};

}
