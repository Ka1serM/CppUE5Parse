#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "MetasoundChannelAgnosticAutoTypes.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EMetasoundChannelAgnosticSourceFormatChooser : uint8 { Auto, Custom };

}
