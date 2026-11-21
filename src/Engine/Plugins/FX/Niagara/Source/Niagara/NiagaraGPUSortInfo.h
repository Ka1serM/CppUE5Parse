#pragma once


#include "NiagaraGPUSortInfo.generated.h"

namespace ue5parse::runtime {

UENUM()
enum class ENiagaraSortMode : uint8 { None, ViewDepth, ViewDistance, CustomAscending, CustomDecending };

}
