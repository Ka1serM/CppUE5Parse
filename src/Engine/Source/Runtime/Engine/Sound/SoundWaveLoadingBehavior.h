#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/NoExportTypes.h"

#include "SoundWaveLoadingBehavior.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ESoundWaveLoadingBehavior : uint8 {
    Inherited = 0,
    RetainOnLoad = 1,
    PrimeOnLoad = 2,
    LoadOnDemand = 3,
    ForceInline = 4,
    Uninitialized = 0xff,
};

}
