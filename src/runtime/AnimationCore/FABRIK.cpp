#include "ue5parse/runtime/AnimationCore/FABRIK.h"

namespace ue5parse::runtime::AnimationCore {

bool SolveFabrik(TArray<FFABRIKChainLink>& InOutChain, const FVector& TargetPosition, double MaximumReach, double Precision, int32 MaxIterations) {
    const int32 NumChainLinks = InOutChain.Num();
    if (FVector::DistSquared(InOutChain[0].Position, TargetPosition) > FMath::Square(MaximumReach)) {
        for (int32 LinkIndex = 1; LinkIndex < NumChainLinks; LinkIndex++) {
            const FFABRIKChainLink& ParentLink = InOutChain[LinkIndex - 1];
            FFABRIKChainLink& CurrentLink = InOutChain[LinkIndex];
            CurrentLink.Position = ParentLink.Position + (TargetPosition - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length;
        }
        return true;
    }
    const int32 TipBoneLinkIndex = NumChainLinks - 1;
    double Slop = FVector::Dist(InOutChain[TipBoneLinkIndex].Position, TargetPosition);
    if (Slop <= Precision) return false;
    InOutChain[TipBoneLinkIndex].Position = TargetPosition;
    int32 IterationCount = 0;
    while (Slop > Precision && IterationCount++ < MaxIterations) {
        for (int32 LinkIndex = TipBoneLinkIndex - 1; LinkIndex > 0; LinkIndex--) {
            FFABRIKChainLink& CurrentLink = InOutChain[LinkIndex];
            const FFABRIKChainLink& ChildLink = InOutChain[LinkIndex + 1];
            CurrentLink.Position = ChildLink.Position + (CurrentLink.Position - ChildLink.Position).GetUnsafeNormal() * ChildLink.Length;
        }
        for (int32 LinkIndex = 1; LinkIndex < TipBoneLinkIndex; LinkIndex++) {
            const FFABRIKChainLink& ParentLink = InOutChain[LinkIndex - 1];
            FFABRIKChainLink& CurrentLink = InOutChain[LinkIndex];
            CurrentLink.Position = ParentLink.Position + (CurrentLink.Position - ParentLink.Position).GetUnsafeNormal() * CurrentLink.Length;
        }
        Slop = FMath::Abs(InOutChain[TipBoneLinkIndex].Length - FVector::Dist(InOutChain[TipBoneLinkIndex - 1].Position, TargetPosition));
    }
    const FFABRIKChainLink& ParentLink = InOutChain[TipBoneLinkIndex - 1];
    FFABRIKChainLink& TipLink = InOutChain[TipBoneLinkIndex];
    TipLink.Position = ParentLink.Position + (TipLink.Position - ParentLink.Position).GetUnsafeNormal() * TipLink.Length;
    return true;
}

}
