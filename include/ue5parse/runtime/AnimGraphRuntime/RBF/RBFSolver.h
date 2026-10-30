#pragma once

#include "ue5parse/runtime/Core/Containers/EnumAsByte.h"
#include "ue5parse/runtime/Engine/Animation/AnimTypes.h"
#include "ue5parse/runtime/Engine/Curves/RichCurve.h"

#include "RBFSolver.generated.h"

namespace ue5parse::runtime {

enum class ERBFSolverType : uint8 { Additive, Interpolative };

enum class ERBFFunctionType : uint8 { Gaussian, Exponential, Linear, Cubic, Quintic, DefaultFunction };

enum class ERBFDistanceMethod : uint8 { Euclidean, Quaternion, SwingAngle, TwistAngle, DefaultMethod };

enum class ERBFNormalizeMethod : uint8 { OnlyNormalizeAboveOne, AlwaysNormalize, NormalizeWithinMedian, NoNormalization };

// FRBFEntry: a point of the RBF space, three values per source: a rotator's roll, pitch and yaw in degrees, or a
// vector's X, Y and Z (UE's RBF/RBFSolver.h).
struct FRBFEntry {
    TArray<float> Values;

    FRotator AsRotator(int32 Index) const;
    FQuat AsQuat(int32 Index) const { return AsRotator(Index).Quaternion(); }
    void AddFromRotator(const FRotator& InRot);
    void AddFromVector(const FVector& InVector);
    int32 GetDimensions() const { return Values.Num(); }
};

// FRBFTarget: a target point and how it activates. The overrides apply to the additive solver only.
struct FRBFTarget : public FRBFEntry {
    float ScaleFactor = 1.f;
    bool bApplyCustomCurve = false;
    FRichCurve CustomCurve;
    ERBFDistanceMethod DistanceMethod = ERBFDistanceMethod::DefaultMethod;
    ERBFFunctionType FunctionType = ERBFFunctionType::DefaultFunction;
};

struct FRBFOutputWeight {
    int32 TargetIndex = 0;
    float TargetWeight = 0.f;
};

USTRUCT()
struct FRBFParams {
    GENERATED_BODY()
    UPROPERTY()
    int32 TargetDimensions = 3;
    UPROPERTY()
    ERBFSolverType SolverType = ERBFSolverType::Additive;
    UPROPERTY()
    float Radius = 1.f;
    UPROPERTY()
    bool bAutomaticRadius = false;
    UPROPERTY()
    ERBFFunctionType Function = ERBFFunctionType::Gaussian;
    UPROPERTY()
    ERBFDistanceMethod DistanceMethod = ERBFDistanceMethod::Euclidean;
    UPROPERTY()
    TEnumAsByte<EBoneAxis> TwistAxis = BA_X;
    UPROPERTY()
    float WeightThreshold = UE_KINDA_SMALL_NUMBER;
    UPROPERTY()
    ERBFNormalizeMethod NormalizeMethod = ERBFNormalizeMethod::OnlyNormalizeAboveOne;
    UPROPERTY()
    FVector MedianReference = FVector::ZeroVector;
    UPROPERTY()
    float MedianMin = 45.0f;
    UPROPERTY()
    float MedianMax = 60.0f;

    FVector GetTwistAxisVector() const;
};

// The radial basis function solver (UE's FRBFSolver) with the additive solver only. The interpolative solver solves a
// linear system over the targets; no cooked Valorant graph uses it, so Solve gives it no weights.
struct FRBFSolver {
    // The weights of the targets for the input, normalized by the params' method, without the ones at or below the
    // weight threshold. Appends to OutputWeights.
    static void Solve(const FRBFParams& Params, const TArray<FRBFTarget>& Targets, const FRBFEntry& Input, TArray<FRBFOutputWeight>& OutputWeights);
    // The distance between two entries in degrees, with the method of the params unless OverrideMethod is set.
    static float FindDistanceBetweenEntries(const FRBFEntry& A, const FRBFEntry& B, const FRBFParams& Params,
                                            ERBFDistanceMethod OverrideMethod = ERBFDistanceMethod::DefaultMethod);
    static float GetRadiusForTarget(const FRBFTarget& Target, const FRBFParams& Params);
};

}
