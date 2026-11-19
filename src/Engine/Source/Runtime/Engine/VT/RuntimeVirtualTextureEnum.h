#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "RuntimeVirtualTextureEnum.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ERuntimeVirtualTextureMainPassType : uint8 { Never, Exclusive, Always };

}
