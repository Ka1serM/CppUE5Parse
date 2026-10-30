#include "ue5parse/runtime/AnimGraphRuntime/RBF/RBFSolver.h"

namespace ue5parse::runtime {

FRotator FRBFEntry::AsRotator(int32 Index) const {
    const int32 BaseIndex = Index * 3;
    if (Values.Num() < BaseIndex + 3) return FRotator::ZeroRotator;
    return FRotator(Values[BaseIndex + 1], Values[BaseIndex + 2], Values[BaseIndex + 0]);
}

void FRBFEntry::AddFromRotator(const FRotator& InRot) {
    Values.Add(static_cast<float>(InRot.Roll));
    Values.Add(static_cast<float>(InRot.Pitch));
    Values.Add(static_cast<float>(InRot.Yaw));
}

void FRBFEntry::AddFromVector(const FVector& InVector) {
    Values.Add(static_cast<float>(InVector.X));
    Values.Add(static_cast<float>(InVector.Y));
    Values.Add(static_cast<float>(InVector.Z));
}

FVector FRBFParams::GetTwistAxisVector() const {
    switch (TwistAxis.GetValue()) {
        case BA_Y: return FVector(0.0, 1.0, 0.0);
        case BA_Z: return FVector(0.0, 0.0, 1.0);
        default: return FVector(1.0, 0.0, 0.0);
    }
}

// The distance between two entries in radians: the root of the sum of the squared distances of each rotation.
static float GetDistanceBetweenEntries(const FRBFEntry& A, const FRBFEntry& B, ERBFDistanceMethod DistanceMethod, const FVector& TwistAxis) {
    float TotalDistance = 0.0f;
    for (int32 Index = 0; Index < A.GetDimensions() / 3; Index++) {
        double Distance = 0.0;
        switch (DistanceMethod) {
            case ERBFDistanceMethod::Euclidean: {
                const FRotator RotA = A.AsRotator(Index), RotB = B.AsRotator(Index);
                Distance = FVector::Distance(FVector(FMath::DegreesToRadians(RotA.Roll), FMath::DegreesToRadians(RotA.Pitch), FMath::DegreesToRadians(RotA.Yaw)),
                                             FVector(FMath::DegreesToRadians(RotB.Roll), FMath::DegreesToRadians(RotB.Pitch), FMath::DegreesToRadians(RotB.Yaw)));
                break;
            }
            case ERBFDistanceMethod::Quaternion:
                Distance = A.AsQuat(Index).GetNormalized().AngularDistance(B.AsQuat(Index).GetNormalized());
                break;
            case ERBFDistanceMethod::TwistAngle:
                Distance = FMath::Abs(A.AsQuat(Index).GetTwistAngle(TwistAxis) - B.AsQuat(Index).GetTwistAngle(TwistAxis));
                break;
            case ERBFDistanceMethod::SwingAngle:
            case ERBFDistanceMethod::DefaultMethod: {
                FQuat SwingA, SwingB, Twist;
                A.AsQuat(Index).ToSwingTwist(TwistAxis, SwingA, Twist);
                B.AsQuat(Index).ToSwingTwist(TwistAxis, SwingB, Twist);
                Distance = SwingA.AngularDistance(SwingB);
                break;
            }
        }
        TotalDistance += FMath::Square(static_cast<float>(Distance));
    }
    return FMath::Sqrt(TotalDistance);
}

float FRBFSolver::FindDistanceBetweenEntries(const FRBFEntry& A, const FRBFEntry& B, const FRBFParams& Params, ERBFDistanceMethod OverrideMethod) {
    const ERBFDistanceMethod DistanceMethod = OverrideMethod == ERBFDistanceMethod::DefaultMethod ? Params.DistanceMethod : OverrideMethod;
    return FMath::RadiansToDegrees(GetDistanceBetweenEntries(A, B, DistanceMethod, Params.GetTwistAxisVector()));
}

float FRBFSolver::GetRadiusForTarget(const FRBFTarget& Target, const FRBFParams& Params) {
    float Radius = Params.Radius;
    if (Params.SolverType == ERBFSolverType::Additive) Radius *= Target.ScaleFactor;
    return FMath::Max(Radius, UE_KINDA_SMALL_NUMBER);
}

// The falloff of the additive solver over a distance in radii. These are the original kernels of the additive solver,
// which differ from the kernels of the interpolative one for linear, Gaussian and exponential.
static float GetAdditiveWeight(float Value, ERBFFunctionType FunctionType) {
    if (Value < 0.0f) return 0.0f;
    switch (FunctionType) {
        case ERBFFunctionType::Gaussian: return FMath::Exp(-Value * Value);
        case ERBFFunctionType::Exponential: return 1.f / FMath::Exp(Value);
        case ERBFFunctionType::Cubic: return FMath::Max(1.f - Value * Value * Value, 0.f);
        case ERBFFunctionType::Quintic: return FMath::Max(1.f - FMath::Pow(Value, 5.0f), 0.f);
        default: return FMath::Max(1.0f - Value, 0.0f);
    }
}

// The factor that normalizes the weights: a sum above 1 always scales to 1; a smaller sum by the params' method.
static float GetWeightScale(const FRBFParams& Params, const FRBFEntry& Input, float TotalWeight) {
    if (TotalWeight > 1.f) return 1.f / TotalWeight;
    switch (Params.NormalizeMethod) {
        case ERBFNormalizeMethod::AlwaysNormalize: return 1.f / TotalWeight;
        case ERBFNormalizeMethod::NormalizeWithinMedian: {
            if (Params.MedianMax < Params.MedianMin) return 1.f;
            FRBFEntry MedianEntry;
            while (MedianEntry.GetDimensions() < Input.GetDimensions()) MedianEntry.AddFromVector(Params.MedianReference);
            const float MedianDistance = FRBFSolver::FindDistanceBetweenEntries(Input, MedianEntry, Params);
            if (MedianDistance > Params.MedianMax) return 1.f;
            if (MedianDistance <= Params.MedianMin) return 1.f / TotalWeight;
            const float Bias = FMath::Clamp((MedianDistance - Params.MedianMin) / (Params.MedianMax - Params.MedianMin), 0.f, 1.f);
            return FMath::Lerp(1.f / TotalWeight, 1.f, Bias);
        }
        default: return 1.f;
    }
}

void FRBFSolver::Solve(const FRBFParams& Params, const TArray<FRBFTarget>& Targets, const FRBFEntry& Input, TArray<FRBFOutputWeight>& OutputWeights) {
    if (Params.TargetDimensions != Input.GetDimensions() || Params.SolverType != ERBFSolverType::Additive) return;
    thread_local TArray<float> AllWeights;
    AllWeights.Reset();
    float TotalWeight = 0.f;
    for (const FRBFTarget& Target : Targets) {
        const ERBFFunctionType FunctionType = Target.FunctionType == ERBFFunctionType::DefaultFunction ? Params.Function : Target.FunctionType;
        const float Distance = FindDistanceBetweenEntries(Target, Input, Params, Target.DistanceMethod);
        float Weight = GetAdditiveWeight(Distance / GetRadiusForTarget(Target, Params), FunctionType);
        if (Target.bApplyCustomCurve) Weight = Target.CustomCurve.Eval(Weight, Weight);
        AllWeights.Add(Weight);
        TotalWeight += Weight;
    }
    if (TotalWeight <= UE_KINDA_SMALL_NUMBER) return;
    const float WeightScale = GetWeightScale(Params, Input, TotalWeight);
    for (int32 TargetIndex = 0; TargetIndex < Targets.Num(); TargetIndex++) {
        const float NormalizedWeight = AllWeights[TargetIndex] * WeightScale;
        if (NormalizedWeight > Params.WeightThreshold) OutputWeights.Add({TargetIndex, NormalizedWeight});
    }
}

}
