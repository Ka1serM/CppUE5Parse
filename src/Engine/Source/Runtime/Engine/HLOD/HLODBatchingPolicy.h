#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "HLODBatchingPolicy.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class EHLODBatchingPolicy : uint8 { None, MeshSection, Instancing };

}
