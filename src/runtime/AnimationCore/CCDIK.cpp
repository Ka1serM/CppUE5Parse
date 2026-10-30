#include "ue5parse/runtime/AnimationCore/CCDIK.h"

namespace ue5parse::runtime::AnimationCore {

static bool UpdateChainLink(TArray<FCCDIKChainLink>& Chain, int32 LinkIndex, const FVector& TargetPos, bool bEnableRotationLimit,
                            const TArray<float>& RotationLimitPerJoints) {
    const int32 TipBoneLinkIndex = Chain.Num() - 1;
    FCCDIKChainLink& CurrentLink = Chain[LinkIndex];
    FTransform& CurrentLinkTransform = CurrentLink.Transform;
    FVector ToEnd = Chain[TipBoneLinkIndex].Transform.GetLocation() - CurrentLinkTransform.GetLocation();
    FVector ToTarget = TargetPos - CurrentLinkTransform.GetLocation();
    ToEnd.Normalize();
    ToTarget.Normalize();
    const double RotationLimitInRadian = FMath::DegreesToRadians(double(RotationLimitPerJoints[LinkIndex]));
    double Angle = FMath::ClampAngle(FMath::Acos(FVector::DotProduct(ToEnd, ToTarget)), -RotationLimitInRadian, RotationLimitInRadian);
    const bool bCanRotate = FMath::Abs(Angle) > UE_DOUBLE_KINDA_SMALL_NUMBER && (!bEnableRotationLimit || RotationLimitInRadian > CurrentLink.CurrentAngleDelta);
    if (!bCanRotate) return false;
    if (bEnableRotationLimit) {
        if (RotationLimitInRadian < CurrentLink.CurrentAngleDelta + Angle) {
            Angle = RotationLimitInRadian - CurrentLink.CurrentAngleDelta;
            if (Angle <= UE_DOUBLE_KINDA_SMALL_NUMBER) return false;
        }
        CurrentLink.CurrentAngleDelta += Angle;
    }
    FVector RotationAxis = FVector::CrossProduct(ToEnd, ToTarget);
    if (RotationAxis.SizeSquared() <= 0.0) return false;
    RotationAxis.Normalize();
    FQuat NewRotation = FQuat(RotationAxis, Angle) * CurrentLinkTransform.GetRotation();
    NewRotation.Normalize();
    CurrentLinkTransform.SetRotation(NewRotation);
    if (LinkIndex > 0) {
        CurrentLink.LocalTransform = CurrentLinkTransform.GetRelativeTransform(Chain[LinkIndex - 1].Transform);
        CurrentLink.LocalTransform.NormalizeRotation();
    }
    FTransform CurrentParentTransform = CurrentLinkTransform;
    for (int32 ChildLinkIndex = LinkIndex + 1; ChildLinkIndex <= TipBoneLinkIndex; ChildLinkIndex++) {
        FCCDIKChainLink& ChildLink = Chain[ChildLinkIndex];
        ChildLink.Transform = ChildLink.LocalTransform * CurrentParentTransform;
        ChildLink.Transform.NormalizeRotation();
        CurrentParentTransform = ChildLink.Transform;
    }
    return true;
}

bool SolveCCDIK(TArray<FCCDIKChainLink>& InOutChain, const FVector& TargetPosition, float Precision, int32 MaxIteration, bool bStartFromTail,
                bool bEnableRotationLimit, const TArray<float>& RotationLimitPerJoints) {
    const int32 TipBoneLinkIndex = InOutChain.Num() - 1;
    bool bBoneLocationUpdated = false;
    // As UE, the flag is not reset between passes.
    bool bLocalUpdated = false;
    double Distance = FVector::Dist(TargetPosition, InOutChain[TipBoneLinkIndex].Transform.GetLocation());
    int32 IterationCount = 0;
    while (Distance > Precision && IterationCount++ < MaxIteration) {
        if (bStartFromTail) {
            for (int32 LinkIndex = TipBoneLinkIndex - 1; LinkIndex > 0; LinkIndex--)
                bLocalUpdated |= UpdateChainLink(InOutChain, LinkIndex, TargetPosition, bEnableRotationLimit, RotationLimitPerJoints);
        } else {
            for (int32 LinkIndex = 1; LinkIndex < TipBoneLinkIndex; LinkIndex++)
                bLocalUpdated |= UpdateChainLink(InOutChain, LinkIndex, TargetPosition, bEnableRotationLimit, RotationLimitPerJoints);
        }
        Distance = FVector::Dist(InOutChain[TipBoneLinkIndex].Transform.GetLocation(), TargetPosition);
        bBoneLocationUpdated |= bLocalUpdated;
        if (!bLocalUpdated) break;
    }
    return bBoneLocationUpdated;
}

}
