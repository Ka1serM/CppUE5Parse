#include "ue5parse/runtime/Engine/Curves/CurveBase.h"

#include "ue5parse/runtime/Engine/Curves/CurveFloat.h"
#include "ue5parse/runtime/Engine/Curves/CurveLinearColor.h"
#include "ue5parse/runtime/Engine/Curves/CurveVector.h"

namespace ue5parse::runtime {

void UCurveBase::GetTimeRange(float& MinTime, float& MaxTime) const {
    bool bFound = false;
    MinTime = MaxTime = 0.f;
    for (int32 Index = 0; Index < GetNumCurves(); Index++) {
        const FRichCurve* Curve = GetCurve(Index);
        if (Curve == nullptr || Curve->Keys.Num() == 0) continue;
        float CurveMin, CurveMax;
        Curve->GetTimeRange(CurveMin, CurveMax);
        MinTime = bFound ? FMath::Min(MinTime, CurveMin) : CurveMin;
        MaxTime = bFound ? FMath::Max(MaxTime, CurveMax) : CurveMax;
        bFound = true;
    }
}

void UCurveBase::GetValueRange(float& MinValue, float& MaxValue) const {
    bool bFound = false;
    MinValue = MaxValue = 0.f;
    for (int32 Index = 0; Index < GetNumCurves(); Index++) {
        const FRichCurve* Curve = GetCurve(Index);
        if (Curve == nullptr || Curve->Keys.Num() == 0) continue;
        float CurveMin, CurveMax;
        Curve->GetValueRange(CurveMin, CurveMax);
        MinValue = bFound ? FMath::Min(MinValue, CurveMin) : CurveMin;
        MaxValue = bFound ? FMath::Max(MaxValue, CurveMax) : CurveMax;
        bFound = true;
    }
}

const FRichCurve* FRuntimeFloatCurve::GetRichCurveConst() const { return ExternalCurve ? &ExternalCurve->FloatCurve : &EditorCurveData; }

FRichCurve* FRuntimeFloatCurve::GetRichCurve() { return ExternalCurve ? &ExternalCurve->FloatCurve : &EditorCurveData; }

FVector FRuntimeVectorCurve::GetValue(float InTime) const {
    if (ExternalCurve) return ExternalCurve->GetVectorValue(InTime);
    return FVector(VectorCurves[0].Eval(InTime), VectorCurves[1].Eval(InTime), VectorCurves[2].Eval(InTime));
}

const FRichCurve* FRuntimeVectorCurve::GetRichCurveConst(int32 Index) const {
    if (Index < 0 || Index >= 3) return nullptr;
    return ExternalCurve ? &ExternalCurve->FloatCurves[Index] : &VectorCurves[Index];
}

FVector UCurveVector::GetVectorValue(float InTime) const {
    return FVector(FloatCurves[0].Eval(InTime), FloatCurves[1].Eval(InTime), FloatCurves[2].Eval(InTime));
}

FLinearColor FRuntimeCurveLinearColor::GetLinearColorValue(float InTime) const {
    if (ExternalCurve) return ExternalCurve->GetLinearColorValue(InTime);
    FLinearColor Result;
    Result.R = ColorCurves[0].Eval(InTime);
    Result.G = ColorCurves[1].Eval(InTime);
    Result.B = ColorCurves[2].Eval(InTime);
    // No alpha keys: opaque.
    Result.A = ColorCurves[3].GetNumKeys() == 0 ? 1.0f : ColorCurves[3].Eval(InTime);
    return Result;
}

FLinearColor UCurveLinearColor::GetUnadjustedLinearColorValue(float InTime) const {
    FLinearColor Result;
    Result.R = FloatCurves[0].Eval(InTime);
    Result.G = FloatCurves[1].Eval(InTime);
    Result.B = FloatCurves[2].Eval(InTime);
    Result.A = FloatCurves[3].GetNumKeys() == 0 ? 1.0f : FloatCurves[3].Eval(InTime);
    return Result;
}

FLinearColor UCurveLinearColor::GetAdjustedLinearColorValue(float InTime, bool bClampValue) const {
    const FLinearColor OriginalColor = GetUnadjustedLinearColorValue(InTime);
    const bool bShouldClampValue = bClampValue || (OriginalColor.R <= 1.0f && OriginalColor.G <= 1.0f && OriginalColor.B <= 1.0f);
    FLinearColor HSVColor = OriginalColor.LinearRGBToHSV();
    float& PixelHue = HSVColor.R;
    float& PixelSaturation = HSVColor.G;
    float& PixelValue = HSVColor.B;
    PixelValue *= AdjustBrightness;
    if (!FMath::IsNearlyEqual(AdjustBrightnessCurve, 1.0f, UE_KINDA_SMALL_NUMBER) && AdjustBrightnessCurve != 0.0f)
        PixelValue = FMath::Pow(PixelValue, AdjustBrightnessCurve);
    if (!FMath::IsNearlyZero(AdjustVibrance, UE_KINDA_SMALL_NUMBER)) {
        const float InvSatRaised = FMath::Pow(1.0f - PixelSaturation, 5.0f);
        const float HalfVibrance = FMath::Clamp(AdjustVibrance, 0.0f, 1.0f) * 0.5f;
        PixelSaturation += HalfVibrance * InvSatRaised;
    }
    PixelSaturation *= AdjustSaturation;
    PixelHue += AdjustHue;
    PixelHue = FMath::Fmod(PixelHue, 360.0f);
    if (PixelHue < 0.0f) PixelHue += 360.0f;
    PixelSaturation = FMath::Clamp(PixelSaturation, 0.0f, 1.0f);
    if (bShouldClampValue) PixelValue = FMath::Clamp(PixelValue, 0.0f, 1.0f);
    FLinearColor LinearColor = HSVColor.HSVToLinearRGB();
    LinearColor.A = FMath::Lerp(AdjustMinAlpha, AdjustMaxAlpha, OriginalColor.A);
    return LinearColor;
}

FLinearColor UCurveLinearColor::GetLinearColorValue(float InTime) const { return GetAdjustedLinearColorValue(InTime, false); }

FLinearColor UCurveLinearColor::GetClampedLinearColorValue(float InTime) const { return GetAdjustedLinearColorValue(InTime, true); }

}
