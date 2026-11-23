#pragma once

#include <unordered_map>
#include <utility>
#include <vector>

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/UObject/NameTypes.h"

namespace ue5parse::runtime {

class FArchive;
class FProperty;
class FSerializedPropertyScope;
class UStruct;

//! The properties the package stores for one struct value, zero values included. A property that is not listed kept the value of the archetype.
class FSerializedPropertyList {
public:
    std::size_t Num() const { return Properties.size(); }
    void Add(const FProperty* Property, int32 ArrayIndex) { Properties.emplace_back(Property, ArrayIndex); }
    void Sort();
    bool Contains(const FProperty* Property, int32 ArrayIndex) const;
    //! A struct has a list only when the package stores it with tagged properties; an array element is keyed by the inner property and its index.
    const FSerializedPropertyList* FindChild(const FProperty* Property, int32 Index) const;

private:
    friend class FSerializedPropertyScope;
    using FKey = std::pair<const FProperty*, int32>;

    FSerializedPropertyList& AddChild(const FProperty* Property, int32 Index);

    std::vector<FKey> Properties;
    std::vector<std::pair<FKey, FSerializedPropertyList>> Children;
};

class FSerializedProperties : public FSerializedPropertyList {
public:
    explicit FSerializedProperties(const void* InContainer) : Container(InContainer) {}

    const void* GetContainer() const { return Container; }
    void Sort();
    const FSerializedPropertyList* FindRow(FName RowName) const;

private:
    friend class FSerializedPropertyScope;
    friend void KeepSerializedProperty(FArchive& Ar, const void* Container, const FProperty* Property, int32 ArrayIndex);
    friend void BeginSerializedStruct(FArchive& Ar, const UStruct* Struct, const void* Data);

    const void* Container;
    FSerializedPropertyScope* Scope = nullptr;
    std::unordered_map<FName, FSerializedPropertyList, FNameHasher> Rows;
};

class FSerializedPropertyScope {
public:
    FSerializedPropertyScope(FArchive& Ar, const void* Owner, const FProperty* Property, int32 ArrayIndex);
    FSerializedPropertyScope(FArchive& Ar, const void* Array, const FProperty* Inner, int32 ElementIndex, const void* Element);
    FSerializedPropertyScope(FArchive& Ar, FName RowName, const UStruct* RowStruct, const void* RowData);
    ~FSerializedPropertyScope();
    FSerializedPropertyScope(const FSerializedPropertyScope&) = delete;
    FSerializedPropertyScope& operator=(const FSerializedPropertyScope&) = delete;

private:
    friend void KeepSerializedProperty(FArchive& Ar, const void* Container, const FProperty* Property, int32 ArrayIndex);
    friend void BeginSerializedStruct(FArchive& Ar, const UStruct* Struct, const void* Data);

    void Open(FArchive& Ar, const void* Owner, const FProperty* InProperty, int32 InIndex, const void* InValue, bool bElement);
    FSerializedPropertyList& GetList();

    FSerializedProperties* Store = nullptr;
    FSerializedPropertyScope* Outer = nullptr;
    FSerializedPropertyScope* Parent = nullptr;
    bool bAttached = false;
    const FProperty* Property = nullptr;
    int32 Index = 0;
    const void* Value = nullptr;
    const UStruct* Struct = nullptr;
    FSerializedPropertyList* List = nullptr;
};

void KeepSerializedProperty(FArchive& Ar, const void* Container, const FProperty* Property, int32 ArrayIndex);
void BeginSerializedStruct(FArchive& Ar, const UStruct* Struct, const void* Data);

}
