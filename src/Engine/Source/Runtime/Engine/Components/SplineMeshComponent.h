#pragma once

#include "Engine/Source/Runtime/Engine/Components/StaticMeshComponent.h"

#include "SplineMeshComponent.generated.h"

namespace ue5parse::runtime {

class UBodySetup;

namespace ESplineMeshAxis {

UENUM()
enum Type : int { X, Y, Z };

}

USTRUCT()
struct FSplineMeshParams {
    GENERATED_BODY()
    UPROPERTY()
    FVector StartPos = FVector::ZeroVector;
    UPROPERTY()
    FVector StartTangent = FVector::ZeroVector;
    UPROPERTY()
    FVector2D StartScale = FVector2D(0, 0);
    UPROPERTY()
    float StartRoll = 0;
    UPROPERTY()
    float EndRoll = 0;
    UPROPERTY()
    FVector2D StartOffset = FVector2D(0, 0);
    UPROPERTY()
    FVector EndPos = FVector::ZeroVector;
    UPROPERTY()
    FVector2D EndScale = FVector2D(0, 0);
    UPROPERTY()
    FVector EndTangent = FVector::ZeroVector;
    UPROPERTY()
    FVector2D EndOffset = FVector2D(0, 0);
    UPROPERTY()
    float NaniteClusterBoundsScale = 1.0f;
};

UCLASS()
class USplineMeshComponent : public UStaticMeshComponent {
    GENERATED_BODY()
public:
    UPROPERTY()
    FSplineMeshParams SplineParams;
    UPROPERTY()
    FVector SplineUpDir = FVector(0, 0, 1);
    UPROPERTY()
    float SplineBoundaryMin = 0;
    UPROPERTY()
    FGuid CachedMeshBodySetupGuid;
    UPROPERTY()
    float VirtualTextureMainPassMaxDrawDistance = 0;
    UPROPERTY()
    TObjectPtr<UBodySetup> BodySetup = nullptr;
    UPROPERTY()
    float SplineBoundaryMax = 0;
    UPROPERTY()
    TEnumAsByte<ESplineMeshAxis::Type> ForwardAxis = ESplineMeshAxis::X;
    UPROPERTY()
    uint8 bAllowSplineEditingPerInstance : 1 = false;
    UPROPERTY()
    uint8 bSmoothInterpRollScale : 1 = false;
    UPROPERTY(transient)
    uint8 bMeshDirty : 1 = false;
    UPROPERTY()
    uint8 bNeverNeedsCookedCollisionData : 1 = false;

    explicit USplineMeshComponent(const FObjectInitializer& ObjectInitializer);

    void Serialize(FArchive& Ar) override;

    UFUNCTION(BlueprintCallable)
    void UpdateMesh() {}
    UFUNCTION(BlueprintCallable)
    FVector GetStartPosition() const { return SplineParams.StartPos; }
    UFUNCTION(BlueprintCallable)
    void SetStartPosition(FVector StartPos, bool bUpdateMesh = true) { SplineParams.StartPos = StartPos; }
    UFUNCTION(BlueprintCallable)
    FVector GetStartTangent() const { return SplineParams.StartTangent; }
    UFUNCTION(BlueprintCallable)
    void SetStartTangent(FVector StartTangent, bool bUpdateMesh = true) { SplineParams.StartTangent = StartTangent; }
    UFUNCTION(BlueprintCallable)
    FVector GetEndPosition() const { return SplineParams.EndPos; }
    UFUNCTION(BlueprintCallable)
    void SetEndPosition(FVector EndPos, bool bUpdateMesh = true) { SplineParams.EndPos = EndPos; }
    UFUNCTION(BlueprintCallable)
    FVector GetEndTangent() const { return SplineParams.EndTangent; }
    UFUNCTION(BlueprintCallable)
    void SetEndTangent(FVector EndTangent, bool bUpdateMesh = true) { SplineParams.EndTangent = EndTangent; }
    UFUNCTION(BlueprintCallable)
    void SetStartAndEnd(FVector StartPos, FVector StartTangent, FVector EndPos, FVector EndTangent, bool bUpdateMesh = true);
    UFUNCTION(BlueprintCallable)
    FVector2D GetStartScale() const { return SplineParams.StartScale; }
    UFUNCTION(BlueprintCallable)
    void SetStartScale(FVector2D StartScale = FVector2D(1, 1), bool bUpdateMesh = true) { SplineParams.StartScale = StartScale; }
    UFUNCTION(BlueprintCallable)
    float GetStartRoll() const { return SplineParams.StartRoll; }
    UFUNCTION(BlueprintCallable)
    void SetStartRoll(float StartRoll, bool bUpdateMesh = true) { SplineParams.StartRoll = StartRoll; }
    UFUNCTION(BlueprintCallable)
    void SetStartRollDegrees(float StartRollDegrees, bool bUpdateMesh = true) { SetStartRoll(FMath::DegreesToRadians(StartRollDegrees)); }
    UFUNCTION(BlueprintCallable)
    FVector2D GetStartOffset() const { return SplineParams.StartOffset; }
    UFUNCTION(BlueprintCallable)
    void SetStartOffset(FVector2D StartOffset, bool bUpdateMesh = true) { SplineParams.StartOffset = StartOffset; }
    UFUNCTION(BlueprintCallable)
    FVector2D GetEndScale() const { return SplineParams.EndScale; }
    UFUNCTION(BlueprintCallable)
    void SetEndScale(FVector2D EndScale = FVector2D(1, 1), bool bUpdateMesh = true) { SplineParams.EndScale = EndScale; }
    UFUNCTION(BlueprintCallable)
    float GetEndRoll() const { return SplineParams.EndRoll; }
    UFUNCTION(BlueprintCallable)
    void SetEndRoll(float EndRoll, bool bUpdateMesh = true) { SplineParams.EndRoll = EndRoll; }
    UFUNCTION(BlueprintCallable)
    void SetEndRollDegrees(float EndRollDegrees, bool bUpdateMesh = true) { SetEndRoll(FMath::DegreesToRadians(EndRollDegrees)); }
    UFUNCTION(BlueprintCallable)
    FVector2D GetEndOffset() const { return SplineParams.EndOffset; }
    UFUNCTION(BlueprintCallable)
    void SetEndOffset(FVector2D EndOffset, bool bUpdateMesh = true) { SplineParams.EndOffset = EndOffset; }
    UFUNCTION(BlueprintCallable)
    ESplineMeshAxis::Type GetForwardAxis() const { return ForwardAxis; }
    UFUNCTION(BlueprintCallable)
    void SetForwardAxis(ESplineMeshAxis::Type InForwardAxis, bool bUpdateMesh = true) { ForwardAxis = InForwardAxis; }
    UFUNCTION(BlueprintCallable)
    FVector GetSplineUpDir() const { return SplineUpDir; }
    UFUNCTION(BlueprintCallable)
    void SetSplineUpDir(const FVector& InSplineUpDir, bool bUpdateMesh = true) { SplineUpDir = InSplineUpDir.GetSafeNormal(); }
    UFUNCTION(BlueprintCallable)
    float GetBoundaryMin() const { return SplineBoundaryMin; }
    UFUNCTION(BlueprintCallable)
    void SetBoundaryMin(float InBoundaryMin, bool bUpdateMesh = true) { SplineBoundaryMin = InBoundaryMin; }
    UFUNCTION(BlueprintCallable)
    float GetBoundaryMax() const { return SplineBoundaryMax; }
    UFUNCTION(BlueprintCallable)
    void SetBoundaryMax(float InBoundaryMax, bool bUpdateMesh = true) { SplineBoundaryMax = InBoundaryMax; }

    float ComputeRatioAlongSpline(float DistanceAlong) const;
    void ComputeVisualMeshSplineTRange(float& MinT, float& MaxT) const;
    FBox ComputeDistortedBounds(const FTransform& InLocalToWorld, const FBoxSphereBounds& InMeshBounds,
                                const FBoxSphereBounds* InBoundsToDistort = nullptr) const;
    FTransform CalcSliceTransform(float DistanceAlong) const;
    FTransform CalcSliceTransformAtSplineOffset(float Alpha, float MinT = 0.0f, float MaxT = 1.0f) const;
    static FVector GetAxisMask(ESplineMeshAxis::Type InAxis);

private:
    FBoxSphereBounds GetSourceMeshBounds() const;
};

}
