#pragma once

#include "ue5parse/runtime/Core/Math/Quat.h"
#include "ue5parse/runtime/Core/Math/Vector.h"

namespace ue5parse::runtime::AnimationCore {

// Clamps the Euler angles of the rotation relative to the reference rotation to the limits in degrees (X yaw, Y pitch,
// Z roll). True when the rotation changed.
bool ConstrainAngularRangeUsingEuler(FQuat& InOutQuatRotation, const FQuat& InRefRotation, const FVector& InLimitMinDegrees,
                                     const FVector& InLimitMaxDegrees);

}
