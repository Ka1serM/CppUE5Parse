#pragma once

#include "Engine/Source/Runtime/Core/Math/Rotator.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"

namespace ue5parse::runtime {

struct FMinimalViewInfo {
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    float FOV = 90.0f;
};

}
