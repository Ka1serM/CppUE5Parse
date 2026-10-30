#pragma once

#include "ue5parse/runtime/Core/Templates/TypeHash.h"

// The rendering settings types of UE's Engine/Scene.h that the light components use.

namespace ue5parse::runtime {

// The unit of a local light's intensity.
enum class ELightUnits : uint8 { Unitless, Candelas, Lumens, EV };

}
