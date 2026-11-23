#include "Engine/Source/Runtime/CoreUObject/UObject/SerializedProperties.h"

#include <algorithm>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

namespace {

const UStruct* GetValueStruct(const FProperty* Property) {
    const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
    return StructProperty == nullptr ? nullptr : StructProperty->Struct;
}

}

void FSerializedPropertyList::Sort() {
    std::ranges::sort(Properties);
    //! A struct that the package stores twice keeps the properties of the last copy, as its value does.
    std::ranges::stable_sort(Children, {}, &std::pair<FKey, FSerializedPropertyList>::first);
    const auto Last = std::unique(Children.rbegin(), Children.rend(), [](const auto& A, const auto& B) { return A.first == B.first; });
    Children.erase(Children.begin(), Last.base());
    for (auto& [Key, Child] : Children)
        Child.Sort();
}

bool FSerializedPropertyList::Contains(const FProperty* Property, int32 ArrayIndex) const {
    return std::ranges::binary_search(Properties, std::pair(Property, ArrayIndex));
}

const FSerializedPropertyList* FSerializedPropertyList::FindChild(const FProperty* Property, int32 Index) const {
    const FKey Key(Property, Index);
    const auto It = std::ranges::lower_bound(Children, Key, {}, &std::pair<FKey, FSerializedPropertyList>::first);
    return It == Children.end() || It->first != Key ? nullptr : &It->second;
}

FSerializedPropertyList& FSerializedPropertyList::AddChild(const FProperty* Property, int32 Index) {
    return Children.emplace_back(FKey(Property, Index), FSerializedPropertyList()).second;
}

void FSerializedProperties::Sort() {
    FSerializedPropertyList::Sort();
    for (auto& [RowName, Row] : Rows)
        Row.Sort();
}

const FSerializedPropertyList* FSerializedProperties::FindRow(FName RowName) const {
    const auto It = Rows.find(RowName);
    return It == Rows.end() ? nullptr : &It->second;
}

FSerializedPropertyScope::FSerializedPropertyScope(FArchive& Ar, const void* Owner, const FProperty* Property, int32 ArrayIndex) {
    Open(Ar, Owner, Property, ArrayIndex, Property->ContainerPtrToValuePtr<void>(Owner, ArrayIndex), false);
}

FSerializedPropertyScope::FSerializedPropertyScope(FArchive& Ar, const void* Array, const FProperty* Inner, int32 ElementIndex, const void* Element) {
    Open(Ar, Array, Inner, ElementIndex, Element, true);
}

FSerializedPropertyScope::FSerializedPropertyScope(FArchive& Ar, FName RowName, const UStruct* RowStruct, const void* RowData)
    : Store(Ar.GetSerializedProperties()), Value(RowData), Struct(RowStruct) {
    if (Store == nullptr)
        return;
    Outer = std::exchange(Store->Scope, this);
    bAttached = true;
    List = &Store->Rows.insert_or_assign(RowName, FSerializedPropertyList()).first->second;
}

FSerializedPropertyScope::~FSerializedPropertyScope() {
    if (Store != nullptr)
        Store->Scope = Outer;
}

void FSerializedPropertyScope::Open(FArchive& Ar, const void* Owner, const FProperty* InProperty, int32 InIndex, const void* InValue, bool bElement) {
    Store = Ar.GetSerializedProperties();
    if (Store == nullptr)
        return;
    Outer = std::exchange(Store->Scope, this);
    Property = InProperty;
    Index = InIndex;
    Value = InValue;
    Struct = GetValueStruct(InProperty);
    if (Outer == nullptr) {
        bAttached = Store->Container == Owner;
        return;
    }
    const bool bOuterOpen = bElement ? Outer->bAttached : Outer->List != nullptr;
    bAttached = bOuterOpen && Outer->Value == Owner;
    Parent = bAttached ? Outer : nullptr;
}

FSerializedPropertyList& FSerializedPropertyScope::GetList() {
    if (List != nullptr)
        return *List;
    FSerializedPropertyList& ParentList = Parent == nullptr ? *Store : Parent->GetList();
    List = &ParentList.AddChild(Property, Index);
    return *List;
}

void KeepSerializedProperty(FArchive& Ar, const void* Container, const FProperty* Property, int32 ArrayIndex) {
    FSerializedProperties* Store = Ar.GetSerializedProperties();
    if (Store == nullptr)
        return;
    FSerializedPropertyScope* Scope = Store->Scope;
    if (Scope == nullptr) {
        if (Store->Container == Container)
            Store->Add(Property, ArrayIndex);
        return;
    }
    if (Scope->bAttached && Scope->List != nullptr && Scope->Value == Container)
        Scope->List->Add(Property, ArrayIndex);
}

void BeginSerializedStruct(FArchive& Ar, const UStruct* Struct, const void* Data) {
    FSerializedProperties* Store = Ar.GetSerializedProperties();
    if (Store == nullptr || Store->Scope == nullptr)
        return;
    FSerializedPropertyScope* Scope = Store->Scope;
    if (Scope->bAttached && Scope->Value == Data && Scope->Struct == Struct)
        Scope->GetList();
}

}
