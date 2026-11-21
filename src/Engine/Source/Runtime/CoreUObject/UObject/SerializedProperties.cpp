#include "Engine/Source/Runtime/CoreUObject/UObject/SerializedProperties.h"

#include <algorithm>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

void FSerializedProperties::Sort() {
    std::ranges::sort(Properties);
}

bool FSerializedProperties::Contains(const FProperty* Property, int32 ArrayIndex) const {
    return std::ranges::binary_search(Properties, std::pair(Property, ArrayIndex));
}

//! Only the properties of the export itself are kept: a struct inside a set or a map is read into a temporary and has no lasting address.
void KeepSerializedProperty(FArchive& Ar, const void* Container, const FProperty* Property, int32 ArrayIndex) {
    FSerializedProperties* Store = Ar.GetSerializedProperties();
    if (Store != nullptr && Store->GetContainer() == Container)
        Store->Add(Property, ArrayIndex);
}

}
