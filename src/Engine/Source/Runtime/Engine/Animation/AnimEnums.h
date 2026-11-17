#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "AnimEnums.generated.h"

namespace ue5parse::runtime {

namespace ERootMotionRootLock {

UENUM()
enum Type : int32 { RefPose, AnimFirstFrame, Zero };

}

}
