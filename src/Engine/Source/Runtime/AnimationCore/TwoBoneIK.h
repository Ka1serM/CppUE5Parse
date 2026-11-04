#pragma once

#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"

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
