#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "UniversalObjectLocatorResolveParams.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ELocatorResolveFlags : uint8 { None, Load = 1 << 0, Unload = 1 << 1, Async = 1 << 2, WillWait = 1 << 3, AsyncWait = Async | WillWait };

}
