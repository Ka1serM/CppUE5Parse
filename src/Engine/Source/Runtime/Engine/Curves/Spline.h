#pragma once

#include "Engine/Source/Runtime/Core/Math/InterpCurve.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

#include "Spline.generated.h"

namespace ue5parse::runtime {

namespace UE::Spline {

enum class ESplineType : uint8 {
    Unimplemented = 0,
    LegacyTangent = 1,
    Tangent = 2,
};

}

class FLegacyTangentSpline {
public:
    FInterpCurveVector PositionCurve;
    FInterpCurveQuat RotationCurve;
    FInterpCurveVector ScaleCurve;
    FInterpCurveFloat ReparamTable;

    bool Serialize(FArchive& Ar);
};

USTRUCT()
struct FSpline {
    GENERATED_BODY()

    bool Serialize(FArchive& Ar);

    UE::Spline::ESplineType GetCurrentImplementation() const { return CurrentImplementation; }
    const FLegacyTangentSpline& GetLegacySpline() const { return LegacySplineImpl; }

private:
    UE::Spline::ESplineType CurrentImplementation = UE::Spline::ESplineType::Unimplemented;
    FLegacyTangentSpline LegacySplineImpl;
};

template <>
struct TStructOpsTypeTraits<FSpline> : TStructOpsTypeTraitsBase2<FSpline> {
    enum { WithSerializer = true };
};

}
