#pragma once

#include "Engine/Source/Runtime/Core/Math/UnrealMath.h"

namespace ue5parse::runtime::AnimationCore {

FQuat SolveAim(const FTransform& CurrentTransform, const FVector& TargetPosition, const FVector& AimVector, bool bUseUpVector = false,
               const FVector& UpVector = FVector::UpVector, float AimClampInDegree = 0.f);

}
