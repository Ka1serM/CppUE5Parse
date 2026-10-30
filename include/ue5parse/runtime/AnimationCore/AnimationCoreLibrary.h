#pragma once

#include "ue5parse/runtime/Core/Math/UnrealMath.h"

// The solvers of UE's AnimationCore AnimationCoreLibrary.h.

namespace ue5parse::runtime::AnimationCore {

// The rotation that turns the aim vector of a transform toward a target, limited to a cone of AimClampInDegree
// (0 is no limit) and, with bUseUpVector, kept in the plane normal to the up vector. A vector that is not
// normalized gives the identity.
FQuat SolveAim(const FTransform& CurrentTransform, const FVector& TargetPosition, const FVector& AimVector, bool bUseUpVector = false,
               const FVector& UpVector = FVector::UpVector, float AimClampInDegree = 0.f);

}
