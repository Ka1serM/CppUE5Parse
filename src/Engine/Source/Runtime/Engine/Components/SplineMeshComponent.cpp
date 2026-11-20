#include "Engine/Source/Runtime/Engine/Components/SplineMeshComponent.h"

#include "Engine/Source/Runtime/Engine/Engine/StaticMesh.h"
#include "Engine/Source/Runtime/Engine/StaticMeshResources.h"

namespace ue5parse::runtime {

namespace {

FVector3f SplineEvalPos(const FSplineMeshParams& Params, float A) {
    const FVector3f StartPos(Params.StartPos);
    const FVector3f StartTangent(Params.StartTangent);
    const FVector3f EndPos(Params.EndPos);
    const FVector3f EndTangent(Params.EndTangent);
    const float A2 = A * A;
    const float A3 = A2 * A;
    return (((2 * A3) - (3 * A2) + 1) * StartPos) + ((A3 - (2 * A2) + A) * StartTangent) + ((A3 - A2) * EndTangent) + (((-2 * A3) + (3 * A2)) * EndPos);
}

FVector3f SplineEvalTangent(const FSplineMeshParams& Params, float A) {
    const FVector3f StartPos(Params.StartPos);
    const FVector3f StartTangent(Params.StartTangent);
    const FVector3f EndPos(Params.EndPos);
    const FVector3f EndTangent(Params.EndTangent);
    const FVector3f C = (6 * StartPos) + (3 * StartTangent) + (3 * EndTangent) - (6 * EndPos);
    const FVector3f D = (-6 * StartPos) - (4 * StartTangent) - (2 * EndTangent) + (6 * EndPos);
    return (C * (A * A)) + (D * A) + StartTangent;
}

}

USplineMeshComponent::USplineMeshComponent(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
    Mobility = EComponentMobility::Static;
    BodyInstance.CollisionProfileName = FName("NoCollision");
    bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::Yes;
    SplineParams.StartTangent = FVector(100, 0, 0);
    SplineParams.StartScale = FVector2D(1, 1);
    SplineParams.EndPos = FVector(100, 0, 0);
    SplineParams.EndTangent = FVector(100, 0, 0);
    SplineParams.EndScale = FVector2D(1, 1);
}

void USplineMeshComponent::Serialize(FArchive& Ar) {
    Super::Serialize(Ar);
    if (Ar.IsError() || Ar.UEVer() >= EUnrealEngineObjectUE4Version::SPLINE_MESH_ORIENTATION)
        return;
    ForwardAxis = ESplineMeshAxis::Z;
    SplineParams.StartRoll -= UE_HALF_PI;
    SplineParams.EndRoll -= UE_HALF_PI;
    SplineParams.StartOffset = FVector2D(-SplineParams.StartOffset.Y, SplineParams.StartOffset.X);
    SplineParams.EndOffset = FVector2D(-SplineParams.EndOffset.Y, SplineParams.EndOffset.X);
}

void USplineMeshComponent::SetStartAndEnd(FVector StartPos, FVector StartTangent, FVector EndPos, FVector EndTangent, bool bUpdateMesh) {
    SplineParams.StartPos = StartPos;
    SplineParams.StartTangent = StartTangent;
    SplineParams.EndPos = EndPos;
    SplineParams.EndTangent = EndTangent;
}

FVector USplineMeshComponent::GetAxisMask(ESplineMeshAxis::Type InAxis) {
    FVector Mask(1);
    Mask[InAxis] = 0;
    return Mask;
}

FBoxSphereBounds USplineMeshComponent::GetSourceMeshBounds() const {
    const UStaticMesh* Mesh = GetStaticMesh();
    if (Mesh == nullptr)
        return {};
    //! UE 5.6 fits the mesh to the source bounds of LOD 0. Older cooks do not store them and fit it to the mesh bounds.
    const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
    if (RenderData == nullptr || RenderData->LODResources.IsEmpty() || !RenderData->LODResources[0].SourceMeshBounds)
        return Mesh->GetBounds();
    return *RenderData->LODResources[0].SourceMeshBounds;
}

float USplineMeshComponent::ComputeRatioAlongSpline(float DistanceAlong) const {
    if (!FMath::IsNearlyEqual(SplineBoundaryMin, SplineBoundaryMax))
        return (DistanceAlong - SplineBoundaryMin) / (SplineBoundaryMax - SplineBoundaryMin);
    const FBoxSphereBounds SourceMeshBounds = GetSourceMeshBounds();
    if (SourceMeshBounds.SphereRadius <= 0)
        return 0;
    const double MeshMinZ = SourceMeshBounds.Origin[ForwardAxis] - SourceMeshBounds.BoxExtent[ForwardAxis];
    const double MeshRangeZ = 2 * SourceMeshBounds.BoxExtent[ForwardAxis];
    if (MeshRangeZ <= UE_SMALL_NUMBER)
        return 0;
    return static_cast<float>((DistanceAlong - MeshMinZ) / MeshRangeZ);
}

void USplineMeshComponent::ComputeVisualMeshSplineTRange(float& MinT, float& MaxT) const {
    MinT = 0;
    MaxT = 1;
    if (FMath::IsNearlyEqual(SplineBoundaryMin, SplineBoundaryMax))
        return;
    const FBoxSphereBounds SourceMeshBounds = GetSourceMeshBounds();
    const float BoundsMin = static_cast<float>(SourceMeshBounds.Origin[ForwardAxis] - SourceMeshBounds.BoxExtent[ForwardAxis]);
    const float BoundsMax = static_cast<float>(SourceMeshBounds.Origin[ForwardAxis] + SourceMeshBounds.BoxExtent[ForwardAxis]);
    const float BoundsMinT = (BoundsMin - SplineBoundaryMin) / (SplineBoundaryMax - SplineBoundaryMin);
    const float BoundsMaxT = (BoundsMax - SplineBoundaryMin) / (SplineBoundaryMax - SplineBoundaryMin);
    constexpr float MaxSplineExtrapolation = 4.0f;
    MinT = FMath::Max(-MaxSplineExtrapolation, BoundsMinT);
    MaxT = FMath::Min(BoundsMaxT, MaxSplineExtrapolation);
}

FTransform USplineMeshComponent::CalcSliceTransform(float DistanceAlong) const {
    float MinT = 0;
    float MaxT = 1;
    ComputeVisualMeshSplineTRange(MinT, MaxT);
    return CalcSliceTransformAtSplineOffset(ComputeRatioAlongSpline(DistanceAlong), MinT, MaxT);
}

FTransform USplineMeshComponent::CalcSliceTransformAtSplineOffset(float Alpha, float MinT, float MaxT) const {
    const float HermiteAlpha = bSmoothInterpRollScale ? static_cast<float>(FMath::SmoothStep(0.0, 1.0, Alpha)) : Alpha;
    FVector3f SplinePos;
    FVector3f SplineDir;
    if (Alpha < MinT) {
        const FVector3f StartTangent = SplineEvalTangent(SplineParams, MinT);
        SplinePos = SplineEvalPos(SplineParams, MinT) + (StartTangent * (Alpha - MinT));
        SplineDir = StartTangent.GetSafeNormal();
    } else if (Alpha > MaxT) {
        const FVector3f EndTangent = SplineEvalTangent(SplineParams, MaxT);
        SplinePos = SplineEvalPos(SplineParams, MaxT) + (EndTangent * (Alpha - MaxT));
        SplineDir = EndTangent.GetSafeNormal();
    } else {
        SplinePos = SplineEvalPos(SplineParams, Alpha);
        SplineDir = SplineEvalTangent(SplineParams, Alpha).GetSafeNormal();
    }
    const FVector3f UpDir(ClampVector(SplineUpDir, -FVector::OneVector, FVector::OneVector));
    const FVector3f BaseXVec = (UpDir ^ SplineDir).GetSafeNormal();
    const FVector3f BaseYVec = (SplineDir ^ BaseXVec).GetSafeNormal();
    const FVector2f SliceOffset = FMath::Lerp(FVector2f(SplineParams.StartOffset), FVector2f(SplineParams.EndOffset), HermiteAlpha);
    SplinePos += SliceOffset.X * BaseXVec;
    SplinePos += SliceOffset.Y * BaseYVec;
    const float UseRoll = FMath::Lerp(SplineParams.StartRoll, SplineParams.EndRoll, HermiteAlpha);
    const float CosAng = static_cast<float>(FMath::Cos(UseRoll));
    const float SinAng = static_cast<float>(FMath::Sin(UseRoll));
    const FVector3f XVec = (CosAng * BaseXVec) - (SinAng * BaseYVec);
    const FVector3f YVec = (CosAng * BaseYVec) + (SinAng * BaseXVec);
    const FVector2f UseScale = FMath::Lerp(FVector2f(SplineParams.StartScale), FVector2f(SplineParams.EndScale), HermiteAlpha);
    const FVector Position(SplinePos);
    switch (ForwardAxis) {
    case ESplineMeshAxis::X: {
        FTransform SliceTransform(FMatrix(FVector(SplineDir), FVector(XVec), FVector(YVec), Position));
        SliceTransform.SetScale3D(FVector(1, UseScale.X, UseScale.Y));
        return SliceTransform;
    }
    case ESplineMeshAxis::Y: {
        FTransform SliceTransform(FMatrix(FVector(YVec), FVector(SplineDir), FVector(XVec), Position));
        SliceTransform.SetScale3D(FVector(UseScale.Y, 1, UseScale.X));
        return SliceTransform;
    }
    case ESplineMeshAxis::Z: {
        FTransform SliceTransform(FMatrix(FVector(XVec), FVector(YVec), FVector(SplineDir), Position));
        SliceTransform.SetScale3D(FVector(UseScale.X, UseScale.Y, 1));
        return SliceTransform;
    }
    }
    return FTransform::Identity;
}

FBox USplineMeshComponent::ComputeDistortedBounds(const FTransform& InLocalToWorld, const FBoxSphereBounds& InMeshBounds,
                                                  const FBoxSphereBounds* InBoundsToDistort) const {
    float MinT = 0;
    float MaxT = 1;
    ComputeVisualMeshSplineTRange(MinT, MaxT);
    const FBoxSphereBounds& BoundsToDistort = InBoundsToDistort ? *InBoundsToDistort : InMeshBounds;
    const bool bExtrapolate = InBoundsToDistort != nullptr && InBoundsToDistort != &InMeshBounds;
    const FVector AxisMask = GetAxisMask(ForwardAxis);
    const FVector FlattenedOrigin = BoundsToDistort.Origin * AxisMask;
    const FVector FlattenedExtent = BoundsToDistort.BoxExtent * AxisMask;
    const FBox FlattenedBounds(FlattenedOrigin - FlattenedExtent, FlattenedOrigin + FlattenedExtent);
    auto GetDistortedSlice = [&](float Alpha) { return FlattenedBounds.TransformBy(CalcSliceTransformAtSplineOffset(Alpha, MinT, MaxT).ToMatrixWithScale()); };

    FBox BoundingBox;
    BoundingBox += GetDistortedSlice(MinT);
    BoundingBox += GetDistortedSlice(MaxT);
    auto AppendAxisExtrema = [&](double Discriminant, double A, double B) {
        if (Discriminant <= 0 || FMath::IsNearlyZero(A))
            return;
        const double SqrtDiscriminant = FMath::Sqrt(Discriminant);
        const double Denominator = 0.5 / A;
        const float T0 = static_cast<float>((-B + SqrtDiscriminant) * Denominator);
        const float T1 = static_cast<float>((-B - SqrtDiscriminant) * Denominator);
        if (T0 > MinT && T0 < MaxT)
            BoundingBox += GetDistortedSlice(T0);
        if (T1 > MinT && T1 < MaxT)
            BoundingBox += GetDistortedSlice(T1);
    };
    const FVector A = 6 * SplineParams.StartPos + 3 * SplineParams.StartTangent + 3 * SplineParams.EndTangent - 6 * SplineParams.EndPos;
    const FVector B = -6 * SplineParams.StartPos - 4 * SplineParams.StartTangent - 2 * SplineParams.EndTangent + 6 * SplineParams.EndPos;
    const FVector Discriminant = B * B - 4 * A * SplineParams.StartTangent;
    AppendAxisExtrema(Discriminant.X, A.X, B.X);
    AppendAxisExtrema(Discriminant.Y, A.Y, B.Y);
    AppendAxisExtrema(Discriminant.Z, A.Z, B.Z);

    if (bExtrapolate) {
        const float StartAlpha = ComputeRatioAlongSpline(static_cast<float>(BoundsToDistort.Origin[ForwardAxis] - BoundsToDistort.BoxExtent[ForwardAxis]));
        if (StartAlpha < MinT)
            BoundingBox += GetDistortedSlice(StartAlpha);
        const float EndAlpha = ComputeRatioAlongSpline(static_cast<float>(BoundsToDistort.Origin[ForwardAxis] + BoundsToDistort.BoxExtent[ForwardAxis]));
        if (EndAlpha > MaxT)
            BoundingBox += GetDistortedSlice(EndAlpha);
    }
    return BoundingBox.TransformBy(InLocalToWorld);
}

}
