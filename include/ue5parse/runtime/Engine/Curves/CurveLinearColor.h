#pragma once

#include "ue5parse/runtime/Engine/Curves/CurveBase.h"

#include "CurveLinearColor.generated.h"

namespace ue5parse::runtime {

class UCurveLinearColor;

USTRUCT()
struct FRuntimeCurveLinearColor {
    GENERATED_BODY()
    UPROPERTY()
    FRichCurve ColorCurves[4];
    UPROPERTY()
    UCurveLinearColor* ExternalCurve = nullptr;
    UPROPERTY()
    bool bDisplayAsLinear = false;

    FLinearColor GetLinearColorValue(float InTime) const;
};

// UCurveLinearColor: a color curve with HSV adjustments on top (UE's Curves/CurveLinearColor.h).
UCLASS()
class UCurveLinearColor : public UCurveBase {
    GENERATED_BODY()
public:
    UPROPERTY()
    FRichCurve FloatCurves[4];
    UPROPERTY()
    float AdjustHue = 0.0f;
    UPROPERTY()
    float AdjustSaturation = 1.0f;
    UPROPERTY()
    float AdjustBrightness = 1.0f;
    UPROPERTY()
    float AdjustBrightnessCurve = 1.0f;
    UPROPERTY()
    float AdjustVibrance = 0.0f;
    UPROPERTY()
    float AdjustMinAlpha = 0.0f;
    UPROPERTY()
    float AdjustMaxAlpha = 1.0f;

    UFUNCTION(BlueprintCallable)
    FLinearColor GetLinearColorValue(float InTime) const;
    FLinearColor GetClampedLinearColorValue(float InTime) const;
    FLinearColor GetUnadjustedLinearColorValue(float InTime) const;
    int32 GetNumCurves() const override { return 4; }
    const FRichCurve* GetCurve(int32 Index) const override { return Index >= 0 && Index < 4 ? &FloatCurves[Index] : nullptr; }

private:
    FLinearColor GetAdjustedLinearColorValue(float InTime, bool bClampValue) const;
};

}
