#include "ue5parse/runtime/AnimationCore/AngularLimit.h"

#include "ue5parse/runtime/Core/Math/Rotator.h"

namespace ue5parse::runtime::AnimationCore {

bool ConstrainAngularRangeUsingEuler(FQuat& InOutQuatRotation, const FQuat& InRefRotation, const FVector& InLimitMinDegrees,
                                     const FVector& InLimitMaxDegrees) {
    const FRotator DeltaRotator = (InRefRotation * InOutQuatRotation.Inverse()).Rotator();
    FRotator NewRotator;
    NewRotator.Pitch = FMath::Clamp(DeltaRotator.Pitch, InLimitMinDegrees.Y, InLimitMaxDegrees.Y);
    NewRotator.Yaw = FMath::Clamp(DeltaRotator.Yaw, InLimitMinDegrees.X, InLimitMaxDegrees.X);
    NewRotator.Roll = FMath::Clamp(DeltaRotator.Roll, InLimitMinDegrees.Z, InLimitMaxDegrees.Z);
    InOutQuatRotation = NewRotator.Quaternion().Inverse() * InRefRotation;
    return !NewRotator.Equals(DeltaRotator);
}

}
