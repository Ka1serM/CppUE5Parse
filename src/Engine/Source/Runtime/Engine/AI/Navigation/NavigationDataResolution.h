#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "NavigationDataResolution.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ENavigationDataResolution : uint8 { Low = 0, Default = 1, High = 2, Invalid = 3, MAX = 3 };

}
