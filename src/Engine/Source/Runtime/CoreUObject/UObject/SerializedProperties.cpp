#include "Engine/Source/Runtime/CoreUObject/UObject/SerializedProperties.h"

#include <algorithm>

#include "Engine/Source/Runtime/Core/Serialization/Archive.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/Class.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

void FSerializedPropertyList::Sort() {
    std::ranges::sort(Properties);
    for (auto& [Key, Child] : Children)
        Child.Sort();
}

bool FSerializedPropertyList::Contains(const FProperty* Property, int32 ArrayIndex) const {
    return std::ranges::binary_search(Properties, std::pair(Property, ArrayIndex));
}

const FSerializedPropertyList* FSerializedPropertyList::FindChild(const FProperty* Property, int32 Index) const {
    const auto It = Children.find({Property, Index});
    return It == Children.end() ? nullptr : &It->second;
}

FSerializedPropertyList* FSerializedProperties::ActiveList(const void* Data) {
    if (Scope)
        return Scope->Container == Data ? Scope->List : nullptr;
    return Container == Data ? this : nullptr;
}

void FSerializedProperties::BeginTaggedStruct(const void* Data) {
    if (auto* List = ActiveList(Data))
        List->bTagged = true;
}

FSerializedPropertyScope::FSerializedPropertyScope(FArchive& Ar, const void* Data, FSerializedPropertyList* InList)
    : Store(Ar.GetSerializedProperties()), Container(Data), List(InList) {
    if (Store)
        Previous = std::exchange(Store->Scope, this);
}

namespace {

bool HasTaggedChildren(const FProperty* Property) {
    if (const auto* Struct = CastField<FStructProperty>(Property)) {
        if (!Struct->Struct)
            return false;
        const auto* Ops = Struct->Struct->GetCppStructOps();
        return !Ops || !Ops->Serialize;
    }
    if (const auto* Array = CastField<FArrayProperty>(Property))
        return HasTaggedChildren(Array->Inner);
    return false;
}

}

FSerializedPropertyScope::FSerializedPropertyScope(FArchive& Ar, const void* Owner, const FProperty* Property, int32 Index, const void* ArrayElement)
    : Store(Ar.GetSerializedProperties()), Container(nullptr), List(nullptr) {
    if (!Store)
        return;
    auto* Parent = Store->ActiveList(Owner);
    Previous = std::exchange(Store->Scope, this);
    if (!Parent)
        return;
    if (!ArrayElement)
        Parent->Add(Property, Index);
    if (!HasTaggedChildren(Property))
        return;
    Container = ArrayElement ? ArrayElement : Property->ContainerPtrToValuePtr<void>(Owner, Index);
    List = &Parent->Children.insert_or_assign({Property, Index}, FSerializedPropertyList{}).first->second;
}

FSerializedPropertyScope::~FSerializedPropertyScope() {
    if (Store)
        Store->Scope = Previous;
}

void KeepSerializedProperty(FArchive& Ar, const void* Container, const FProperty* Property, int32 ArrayIndex) {
    FSerializedProperties* Store = Ar.GetSerializedProperties();
    if (Store && Store->GetContainer() == Container)
        Store->Add(Property, ArrayIndex);
}

}
