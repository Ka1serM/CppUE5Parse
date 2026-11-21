#include "Engine/Plugins/Runtime/PropertyBindingUtils/Source/PropertyBindingUtils/PropertyBindingTypes.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

namespace ue5parse::runtime {

bool FPropertyBindingIndex16::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("UInt16Property"))
        return false;
    Ar << Value;
    return true;
}

}
