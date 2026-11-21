#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "WorldConditionTypes.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EWorldConditionOperator : uint8 { And, Or, Copy };

}
