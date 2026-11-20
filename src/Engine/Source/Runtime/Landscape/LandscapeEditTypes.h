#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "LandscapeEditTypes.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ELandscapeTargetLayerBlendMethod : uint8 {
    None = 0,
    FinalWeightBlending,
    PremultipliedAlphaBlending,
    Count,
};

}
