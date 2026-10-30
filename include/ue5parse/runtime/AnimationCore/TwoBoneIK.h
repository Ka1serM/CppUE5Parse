#pragma once

#include "ue5parse/runtime/Core/Math/UnrealMath.h"

// The two bone IK solver (UE's AnimationCore TwoBoneIK.h): places the joint and the end of a two bone chain so that
// the end reaches the effector, bending toward the joint target.

namespace ue5parse::runtime::AnimationCore {

void SolveTwoBoneIK(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget,
                    const FVector& Effector, FVector& OutJointPos, FVector& OutEndPos, bool bAllowStretching, double StartStretchRatio,
                    double MaxStretchScale);
void SolveTwoBoneIK(const FVector& RootPos, const FVector& JointPos, const FVector& EndPos, const FVector& JointTarget,
                    const FVector& Effector, FVector& OutJointPos, FVector& OutEndPos, double UpperLimbLength, double LowerLimbLength,
                    bool bAllowStretching, double StartStretchRatio, double MaxStretchScale);
void SolveTwoBoneIK(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget,
                    const FVector& Effector, bool bAllowStretching, double StartStretchRatio, double MaxStretchScale);
void SolveTwoBoneIK(FTransform& InOutRootTransform, FTransform& InOutJointTransform, FTransform& InOutEndTransform, const FVector& JointTarget,
                    const FVector& Effector, double UpperLimbLength, double LowerLimbLength, bool bAllowStretching, double StartStretchRatio,
                    double MaxStretchScale);

}
