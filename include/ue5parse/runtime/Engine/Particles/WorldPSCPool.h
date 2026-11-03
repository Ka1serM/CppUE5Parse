#pragma once

#include "ue5parse/runtime/CoreUObject/UObject/ObjectMacros.h"

#include "WorldPSCPool.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EPSCPoolMethod : uint8 { None, AutoRelease, ManualRelease, ManualRelease_OnComplete, FreeInPool };

}
