#include "Engine/Source/Runtime/Engine/WorldPartition/DataLayer/DataLayerInstanceNames.h"

#include "Engine/Source/Runtime/CoreUObject/UObject/PropertyTag.h"

namespace ue5parse::runtime {

bool FDataLayerInstanceNames::SerializeFromMismatchedTag(const FPropertyTag& Tag, FArchive& Ar) {
    if (Tag.Type != FName("ArrayProperty"))
        return false;
    Ar << DataLayers;
    return true;
}

}
