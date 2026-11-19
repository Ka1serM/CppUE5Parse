#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "NavDataGatheringMode.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ENavDataGatheringMode : uint8 { Default, Instant, Lazy };

}
