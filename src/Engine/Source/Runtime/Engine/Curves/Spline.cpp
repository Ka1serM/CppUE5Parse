#include "Engine/Source/Runtime/Engine/Curves/Spline.h"

namespace ue5parse::runtime {

namespace {

template <class T>
void SerializeInterpCurve(FArchive& Ar, FInterpCurve<T>& Curve) {
    int32 NumPoints = 0;
    Ar << NumPoints;
    if (NumPoints < 0) {
        Ar.SetError(ErrorCode::InvalidData, "FInterpCurve.Points");
        return;
    }
    Curve.Points.SetNum(NumPoints);
    for (FInterpCurvePoint<T>& Point : Curve.Points) {
        Ar << Point.InVal << Point.OutVal;
        Ar << Point.ArriveTangent << Point.LeaveTangent;
        Ar << Point.InterpMode;
    }
    if (Ar.UEVer() >= EUnrealEngineObjectUE4Version::INTERPCURVE_SUPPORTS_LOOPING) {
        Ar << Curve.bIsLooped;
        Ar << Curve.LoopKeyOffset;
    }
}

}

bool FLegacyTangentSpline::Serialize(FArchive& Ar) {
    SerializeInterpCurve(Ar, PositionCurve);
    SerializeInterpCurve(Ar, RotationCurve);
    SerializeInterpCurve(Ar, ScaleCurve);
    SerializeInterpCurve(Ar, ReparamTable);
    return true;
}

bool FSpline::Serialize(FArchive& Ar) {
    //! The first byte is the implementation that saved the spline. The bytes after it hold that implementation's data.
    uint8 PreviousImpl = 0;
    Ar << PreviousImpl;
    CurrentImplementation = static_cast<UE::Spline::ESplineType>(PreviousImpl);
    if (CurrentImplementation == UE::Spline::ESplineType::LegacyTangent) {
        LegacySplineImpl.Serialize(Ar);
        return true;
    }
    if (CurrentImplementation == UE::Spline::ESplineType::Tangent)
        Ar.SetError(ErrorCode::InvalidData, "FSpline.TangentSpline");
    return true;
}

}
