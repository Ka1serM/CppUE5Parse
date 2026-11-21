#include "Engine/Plugins/FX/Niagara/Source/Niagara/NiagaraStatelessDistribution.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

namespace ue5parse::runtime {

bool FNiagaraDistributionRangeFloat::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("FloatProperty"))
        return false;
    float Value = 0;
    Ar << Value;
    Mode = ENiagaraDistributionMode::UniformConstant;
    Min = Value;
    Max = Value;
    return true;
}

}
