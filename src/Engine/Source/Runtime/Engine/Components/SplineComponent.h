#pragma once

#include "Engine/Source/Runtime/Core/Math/InterpCurve.h"
#include "Engine/Source/Runtime/Engine/Components/PrimitiveComponent.h"

#include "SplineComponent.generated.h"

namespace ue5parse::runtime {

USTRUCT()
struct FSplineCurves {
    GENERATED_BODY()
    UPROPERTY()
    FInterpCurveVector Position;
    UPROPERTY()
    FInterpCurveQuat Rotation;
    UPROPERTY()
    FInterpCurveVector Scale;
    UPROPERTY()
    FInterpCurveFloat ReparamTable;
};

UCLASS()
class USplineComponent : public UPrimitiveComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FSplineCurves SplineCurves;
    UPROPERTY()
    FInterpCurveVector SplineInfo;
    UPROPERTY()
    FInterpCurveQuat SplineRotInfo;
    UPROPERTY()
    FInterpCurveVector SplineScaleInfo;
    UPROPERTY()
    FInterpCurveFloat SplineReparamTable;
    UPROPERTY()
    int32 ReparamStepsPerSegment = 10;
    UPROPERTY()
    float Duration = 1.0f;
    UPROPERTY()
    bool bStationaryEndpoints = false;
    UPROPERTY()
    bool bSplineHasBeenEdited = false;
    UPROPERTY()
    bool bClosedLoop = false;
    UPROPERTY()
    FVector DefaultUpVector = FVector::UpVector;

    explicit USplineComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
};

}
