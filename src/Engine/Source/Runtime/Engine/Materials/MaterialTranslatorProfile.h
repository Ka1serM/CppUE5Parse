#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyAccess.h"

#include "MaterialTranslatorProfile.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EMaterialTranslatorProfile : uint8 { Compatibility, Strict };

}
