#pragma once

#include "Engine/Source/Runtime/Core/Math/Rotator.h"
#include "Engine/Source/Runtime/Core/Math/Vector.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "CameraTypes.generated.h"

namespace ue5parse::runtime {

namespace ECameraProjectionMode {

UENUM()
enum Type : int32 { Perspective, Orthographic };

}

struct FMinimalViewInfo {
    FVector Location = FVector::ZeroVector;
    FRotator Rotation = FRotator::ZeroRotator;
    float FOV = 90.0f;
};

}
