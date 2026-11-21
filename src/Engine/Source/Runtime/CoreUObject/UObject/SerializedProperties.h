#pragma once

#include <map>
#include <utility>
#include <vector>

#include "Engine/Source/Runtime/Core/Containers/Array.h"

namespace ue5parse::runtime {

class FArchive;
class FProperty;
class FSerializedPropertyScope;

class FSerializedPropertyList {
public:
    std::size_t Num() const { return Properties.size(); }
    void Add(const FProperty* Property, int32 ArrayIndex) { Properties.emplace_back(Property, ArrayIndex); }
    void Sort();
    bool Contains(const FProperty* Property, int32 ArrayIndex) const;
    bool HasTaggedProperties() const { return bTagged; }
    const FSerializedPropertyList* FindChild(const FProperty* Property, int32 Index) const;

private:
    friend class FSerializedPropertyScope;
    friend class FSerializedProperties;
    using FKey = std::pair<const FProperty*, int32>;
    std::vector<FKey> Properties;
    std::map<FKey, FSerializedPropertyList> Children;
    bool bTagged = false;
};

class FSerializedProperties : public FSerializedPropertyList {
public:
    explicit FSerializedProperties(const void* InContainer) : Container(InContainer) {}
    const void* GetContainer() const { return Container; }
    void BeginTaggedStruct(const void* Data);

private:
    friend class FSerializedPropertyScope;
    const void* Container;
    FSerializedPropertyList* ActiveList(const void* Data);
    FSerializedPropertyScope* Scope = nullptr;
};

class FSerializedPropertyScope {
public:
    FSerializedPropertyScope(FArchive& Ar, const void* Owner, const FProperty* Property, int32 Index, const void* ArrayElement = nullptr);
    FSerializedPropertyScope(FArchive& Ar, const void* Data, FSerializedPropertyList* List);
    ~FSerializedPropertyScope();
    FSerializedPropertyScope(const FSerializedPropertyScope&) = delete;
    FSerializedPropertyScope& operator=(const FSerializedPropertyScope&) = delete;

private:
    friend class FSerializedProperties;
    FSerializedProperties* Store;
    const void* Container;
    FSerializedPropertyList* List;
    FSerializedPropertyScope* Previous = nullptr;
};

void KeepSerializedProperty(FArchive& Ar, const void* Container, const FProperty* Property, int32 ArrayIndex);

}
