#include "ue5parse/runtime/AnimationCore/TwoBoneIK.h"

namespace ue5parse::runtime::AnimationCore {

void SolveTwoBoneIK(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget,
                    const FVector& Effector, bool bAllowStretching, double StartStretchRatio, double MaxStretchScale) {
    const double LowerLimbLength = (InOutEndTransform.GetLocation() - InOutJointTransform.GetLocation()).Size();
    const double UpperLimbLength = (InOutJointTransform.GetLocation() - InOutRootTransform.GetLocation()).Size();
    SolveTwoBoneIK(InOutRootTransform, InOutJointTransform, InOutEndTransform, JointTarget, Effector, UpperLimbLength, LowerLimbLength,
                   bAllowStretching, StartStretchRatio, MaxStretchScale);
}

void SolveTwoBoneIK(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget,
                    const FVector& Effector, double UpperLimbLength, double LowerLimbLength, bool bAllowStretching, double StartStretchRatio,
                    double MaxStretchScale) {
    FVector OutJointPos, OutEndPos;
    const FVector RootPos = InOutRootTransform.GetLocation();
    const FVector JointPos = InOutJointTransform.GetLocation();
    const FVector EndPos = InOutEndTransform.GetLocation();
    SolveTwoBoneIK(RootPos, JointPos, EndPos, JointTarget, Effector, OutJointPos, OutEndPos, UpperLimbLength, LowerLimbLength, bAllowStretching,
                   StartStretchRatio, MaxStretchScale);
    // The upper bone turns from its old direction to the new one; the lower bone too, and moves with the joint.
    {
        const FVector OldDir = (JointPos - RootPos).GetSafeNormal();
        const FVector NewDir = (OutJointPos - RootPos).GetSafeNormal();
        const FQuat DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
        InOutRootTransform.SetRotation(DeltaRotation * InOutRootTransform.GetRotation());
        InOutRootTransform.SetTranslation(RootPos);
    }
    {
        const FVector OldDir = (EndPos - JointPos).GetSafeNormal();
        const FVector NewDir = (OutEndPos - OutJointPos).GetSafeNormal();
        const FQuat DeltaRotation = FQuat::FindBetweenNormals(OldDir, NewDir);
        InOutJointTransform.SetRotation(DeltaRotation * InOutJointTransform.GetRotation());
        InOutJointTransform.SetTranslation(OutJointPos);
    }
    InOutEndTransform.SetTranslation(OutEndPos);
}

void SolveTwoBoneIK(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget,
                    const FVector& Effector, FVector& OutJointPos, FVector& OutEndPos, bool bAllowStretching, double StartStretchRatio,
                    double MaxStretchScale) {
    const double LowerLimbLength = (EndPos - JointPos).Size();
    const double UpperLimbLength = (JointPos - RootPos).Size();
    SolveTwoBoneIK(RootPos, JointPos, EndPos, JointTarget, Effector, OutJointPos, OutEndPos, UpperLimbLength, LowerLimbLength,
                   bAllowStretching, StartStretchRatio, MaxStretchScale);
}

void SolveTwoBoneIK(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget,
                    const FVector& Effector, FVector& OutJointPos, FVector& OutEndPos, double UpperLimbLength, double LowerLimbLength,
                    bool bAllowStretching, double StartStretchRatio, double MaxStretchScale) {
    const FVector DesiredPos = Effector;
    const FVector DesiredDelta = DesiredPos - RootPos;
    double DesiredLength = DesiredDelta.Size();
    double MaxLimbLength = LowerLimbLength + UpperLimbLength;
    FVector DesiredDir;
    if (DesiredLength < UE_DOUBLE_KINDA_SMALL_NUMBER) {
        DesiredLength = UE_DOUBLE_KINDA_SMALL_NUMBER;
        DesiredDir = FVector(1, 0, 0);
    } else {
        DesiredDir = DesiredDelta.GetSafeNormal();
    }
    // The plane of the bend: the desired direction and the joint target.
    const FVector JointTargetDelta = JointTarget - RootPos;
    FVector JointPlaneNormal, JointBendDir;
    if (JointTargetDelta.SizeSquared() < FMath::Square(UE_DOUBLE_KINDA_SMALL_NUMBER)) {
        JointBendDir = FVector(0, 1, 0);
        JointPlaneNormal = FVector(0, 0, 1);
    } else {
        JointPlaneNormal = DesiredDir ^ JointTargetDelta;
        if (JointPlaneNormal.SizeSquared() < FMath::Square(UE_DOUBLE_KINDA_SMALL_NUMBER)) {
            DesiredDir.FindBestAxisVectors(JointPlaneNormal, JointBendDir);
        } else {
            JointPlaneNormal.Normalize();
            JointBendDir = JointTargetDelta - ((JointTargetDelta | DesiredDir) * DesiredDir);
            JointBendDir.Normalize();
        }
    }
    if (bAllowStretching) {
        const double ScaleRange = MaxStretchScale - StartStretchRatio;
        if (ScaleRange > UE_DOUBLE_KINDA_SMALL_NUMBER && MaxLimbLength > UE_DOUBLE_KINDA_SMALL_NUMBER) {
            const double ReachRatio = DesiredLength / MaxLimbLength;
            const double ScalingFactor = (MaxStretchScale - 1.0) * FMath::Clamp((ReachRatio - StartStretchRatio) / ScaleRange, 0.0, 1.0);
            if (ScalingFactor > UE_DOUBLE_KINDA_SMALL_NUMBER) {
                LowerLimbLength *= (1.0 + ScalingFactor);
                UpperLimbLength *= (1.0 + ScalingFactor);
                MaxLimbLength *= (1.0 + ScalingFactor);
            }
        }
    }
    OutEndPos = DesiredPos;
    OutJointPos = JointPos;
    if (DesiredLength >= MaxLimbLength) {
        // Out of reach: the chain points straight at the effector.
        OutEndPos = RootPos + (MaxLimbLength * DesiredDir);
        OutJointPos = RootPos + (UpperLimbLength * DesiredDir);
        return;
    }
    // The law of cosines gives the angle at the root.
    const double TwoAB = 2.0 * UpperLimbLength * DesiredLength;
    const double CosAngle = TwoAB != 0.0 ? (UpperLimbLength * UpperLimbLength + DesiredLength * DesiredLength - LowerLimbLength * LowerLimbLength) / TwoAB : 0.0;
    const bool bReverseUpperBone = CosAngle < 0.0;
    const double Angle = FMath::Acos(CosAngle);
    const double JointLineDist = UpperLimbLength * FMath::Sin(Angle);
    const double ProjJointDistSqr = UpperLimbLength * UpperLimbLength - JointLineDist * JointLineDist;
    double ProjJointDist = ProjJointDistSqr > 0.0 ? FMath::Sqrt(ProjJointDistSqr) : 0.0;
    if (bReverseUpperBone) ProjJointDist *= -1.0;
    OutJointPos = RootPos + (ProjJointDist * DesiredDir) + (JointLineDist * JointBendDir);
}

}
