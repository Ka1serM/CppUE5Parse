#pragma once

#include "Engine/Source/Runtime/Core/Templates/TypeHash.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/ObjectMacros.h"

#include "Scene.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ELightUnits : uint8 { Unitless, Candelas, Lumens, EV };

}
