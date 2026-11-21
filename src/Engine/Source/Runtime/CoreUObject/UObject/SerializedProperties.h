#pragma once

#include <utility>
#include <vector>

#include "Engine/Source/Runtime/Core/Containers/Array.h"

namespace ue5parse::runtime {

class FArchive;
class FProperty;

//! The properties the package stores for one export, zero values included. A property that is not listed kept the value of the archetype.
class FSerializedProperties {
public:
    explicit FSerializedProperties(const void* InContainer) : Container(InContainer) {}

    const void* GetContainer() const { return Container; }
    std::size_t Num() const { return Properties.size(); }
    void Add(const FProperty* Property, int32 ArrayIndex) { Properties.emplace_back(Property, ArrayIndex); }
    void Sort();
    bool Contains(const FProperty* Property, int32 ArrayIndex) const;

private:
    const void* Container;
    std::vector<std::pair<const FProperty*, int32>> Properties;
};

void KeepSerializedProperty(FArchive& Ar, const void* Container, const FProperty* Property, int32 ArrayIndex);

}
