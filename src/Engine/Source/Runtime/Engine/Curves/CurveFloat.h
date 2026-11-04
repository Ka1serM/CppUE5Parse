#pragma once

#include "Engine/Source/Runtime/Engine/Curves/CurveBase.h"

#include "CurveFloat.generated.h"

namespace ue5parse::runtime {

class UCurveFloat;

USTRUCT()
struct FRuntimeFloatCurve {
    GENERATED_BODY()
    UPROPERTY()
    FRichCurve EditorCurveData;
    UPROPERTY()
    TObjectPtr<UCurveFloat> ExternalCurve = nullptr;

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
