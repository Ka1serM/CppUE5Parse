#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "AnimEnums.generated.h"

namespace ue5parse::runtime {

UENUM()
enum AnimationKeyFormat : int32 { AKF_ConstantKeyLerp, AKF_VariableKeyLerp, AKF_PerTrackCompression };

UENUM()
enum AnimationCompressionFormat : int32 {
    ACF_None,
    ACF_Float96NoW,
    ACF_Fixed48NoW,
    ACF_IntervalFixed32NoW,
    ACF_Fixed32NoW,
    ACF_Float32NoW,
    ACF_Identity,
};

namespace ERootMotionRootLock {

UENUM()
enum Type : int32 { RefPose, AnimFirstFrame, Zero };

}

}
