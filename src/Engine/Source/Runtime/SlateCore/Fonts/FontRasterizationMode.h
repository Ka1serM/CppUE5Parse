#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "FontRasterizationMode.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EFontRasterizationMode : uint8 { Bitmap, Msdf, Sdf, SdfApproximation };

}
