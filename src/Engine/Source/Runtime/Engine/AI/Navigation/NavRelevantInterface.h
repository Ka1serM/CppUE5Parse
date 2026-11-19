#pragma once

#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"

#include "NavRelevantInterface.generated.h"

namespace ue5parse::runtime {

namespace EHasCustomNavigableGeometry {

UENUM()
enum Type : int32 { No, Yes, EvenIfNotCollidable, DontExport };

}

}
