#pragma once

#include "ue5parse/runtime/Core/Math/Rotator.h"
#include "ue5parse/runtime/Core/Math/Vector.h"

namespace ue5parse::runtime {

// FMinimalViewInfo: the place and the horizontal field of view of a view (UE's Camera/CameraTypes.h). The projection
// settings and the post process settings are left out.
struct FMinimalViewInfo {
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    float FOV = 90.0f;
};

}
