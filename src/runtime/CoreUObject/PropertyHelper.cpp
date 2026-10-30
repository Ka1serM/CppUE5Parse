#include <cstring>

#include "ue5parse/runtime/CoreUObject/UObject/UnrealType.h"

namespace ue5parse::runtime {

FScriptArrayHelper::FScriptArrayHelper(const FProperty* InInner, const void* InArray)
    : Inner(InInner), HeapArray(const_cast<FScriptArray*>(static_cast<const FScriptArray*>(InArray))),
      ElementSize(InInner->ElementSize * InInner->ArrayDim), ElementAlignment(InInner->GetMinAlignment()) {}

void FScriptArrayHelper::ConstructItems(int32 Index, int32 Count) {
    if (Count <= 0) return;
    uint8* Dest = static_cast<uint8*>(HeapArray->GetData()) + static_cast<std::size_t>(Index) * ElementSize;
    if (Inner->PropertyFlags & CPF_ZeroConstructor) {
        std::memset(Dest, 0, static_cast<std::size_t>(Count) * ElementSize);
    } else {
        for (int32 i = 0; i < Count; i++) Inner->InitializeValue(Dest + static_cast<std::size_t>(i) * ElementSize);
    }
}

void FScriptArrayHelper::DestructItems(int32 Index, int32 Count) {
    if (Count <= 0 || (Inner->PropertyFlags & CPF_NoDestructor)) return;
    uint8* Dest = static_cast<uint8*>(HeapArray->GetData()) + static_cast<std::size_t>(Index) * ElementSize;
    for (int32 i = 0; i < Count; i++) Inner->DestroyValue(Dest + static_cast<std::size_t>(i) * ElementSize);
}

int32 FScriptArrayHelper::AddValues(int32 Count) {
    int32 Index = HeapArray->Add(Count, ElementSize, ElementAlignment);
    ConstructItems(Index, Count);
    return Index;
}

void FScriptArrayHelper::InsertValues(int32 Index, int32 Count) {
    HeapArray->Insert(Index, Count, ElementSize, ElementAlignment);
    ConstructItems(Index, Count);
}

void FScriptArrayHelper::RemoveValues(int32 Index, int32 Count) {
    DestructItems(Index, Count);
    HeapArray->Remove(Index, Count, ElementSize, ElementAlignment);
}

void FScriptArrayHelper::EmptyValues(int32 Slack) {
    DestructItems(0, Num());
    HeapArray->Empty(Slack, ElementSize, ElementAlignment);
}

void FScriptArrayHelper::EmptyAndAddValues(int32 Count) {
    EmptyValues(Count);
    AddValues(Count);
}

void FScriptArrayHelper::Resize(int32 NewNum) {
    if (NewNum > Num()) {
        AddValues(NewNum - Num());
    } else if (NewNum < Num()) {
        RemoveValues(NewNum, Num() - NewNum);
    }
}

namespace {

// The element of a set or the key of a map: the hashed part. The hash bucket of each element is stored after it.
int32& HashNextId(uint8* Element, const FScriptSetLayout& Layout) {
    return *reinterpret_cast<int32*>(Element + Layout.HashNextIdOffset);
}
int32& HashIndex(uint8* Element, const FScriptSetLayout& Layout) {
    return *reinterpret_cast<int32*>(Element + Layout.HashIndexOffset);
}

void LinkIntoHash(FScriptSet& Set, const FScriptSetLayout& Layout, int32 Index, uint32 Hash) {
    uint8* Element = static_cast<uint8*>(Set.GetData(Index, Layout));
    HashIndex(Element, Layout) = static_cast<int32>(Hash);
    int32& Bucket = Set.GetHashBucket(Hash);
    HashNextId(Element, Layout) = Bucket;
    Bucket = Index;
}

// The element whose key equals KeyPtr, through the hash chain of its bucket.
int32 FindInHash(const FScriptSet& Set, const FScriptSetLayout& Layout, const FProperty* KeyProp, const void* KeyPtr) {
    if (Set.Num() == 0 || Set.GetHashSize() == 0) return INDEX_NONE;
    uint32 Hash = KeyProp->GetValueTypeHash(KeyPtr);
    for (int32 Index = Set.GetHashBucket(Hash); Index != INDEX_NONE;) {
        uint8* Element = const_cast<uint8*>(static_cast<const uint8*>(Set.GetData(Index, Layout)));
        if (KeyProp->Identical(KeyPtr, Element)) return Index;
        Index = HashNextId(Element, Layout);
    }
    return INDEX_NONE;
}

// After an element is allocated: grows the buckets when the count needs more (rehashing everything), else links it.
template <class THelper>
void LinkOrRehash(THelper& Helper, FScriptSet& Set, const FScriptSetLayout& Layout, int32 Index, uint32 Hash) {
    int32 Desired = FScriptSet::GetNumberOfHashBuckets(Set.Num());
    if (Set.GetHashSize() < Desired) {
        Helper.Rehash();
    } else {
        LinkIntoHash(Set, Layout, Index, Hash);
    }
}

}

FScriptSetHelper FScriptSetHelper::CreateHelperFormElementProperty(const FProperty* InElementProp, const void* InSet) {
    return FScriptSetHelper(InElementProp, GetScriptSetLayout(InElementProp->GetSize(), InElementProp->GetMinAlignment()), InSet);
}

int32 FScriptSetHelper::FindInternalIndex(int32 LogicalIndex) const {
    if (LogicalIndex < 0 || LogicalIndex >= Num()) return INDEX_NONE;
    for (int32 Index = 0; Index < GetMaxIndex(); Index++) {
        if (!IsValidIndex(Index)) continue;
        if (LogicalIndex-- == 0) return Index;
    }
    return INDEX_NONE;
}

int32 FScriptSetHelper::FindElementIndex(const void* ElementToFind) const {
    return FindInHash(*Set, SetLayout, ElementProp, ElementToFind);
}

void FScriptSetHelper::LinkElement(int32 Index, uint32 Hash) { LinkIntoHash(*Set, SetLayout, Index, Hash); }

int32 FScriptSetHelper::AddElement(const void* ElementToAdd) {
    int32 Existing = FindElementIndex(ElementToAdd);
    if (Existing != INDEX_NONE) {
        // UE replaces the equal element with the new one.
        uint8* Element = GetElementPtr(Existing);
        ElementProp->CopySingleValue(Element, ElementToAdd);
        return Existing;
    }
    int32 Index = Set->AddUninitialized(SetLayout);
    uint8* Element = GetElementPtr(Index);
    std::memset(Element, 0, static_cast<std::size_t>(SetLayout.Size));
    ElementProp->InitializeValue(Element);
    ElementProp->CopySingleValue(Element, ElementToAdd);
    LinkOrRehash(*this, *Set, SetLayout, Index, ElementProp->GetValueTypeHash(Element));
    return Index;
}

void FScriptSetHelper::RemoveAt(int32 Index, int32 Count) {
    for (; Count > 0 && Index < GetMaxIndex(); Index++) {
        if (!IsValidIndex(Index)) continue;
        ElementProp->DestroyValue(GetElementPtr(Index));
        Set->RemoveAt(Index, SetLayout);
        Count--;
    }
}

bool FScriptSetHelper::RemoveElement(const void* ElementToRemove) {
    int32 Index = FindElementIndex(ElementToRemove);
    if (Index == INDEX_NONE) return false;
    RemoveAt(Index);
    return true;
}

void FScriptSetHelper::EmptyElements(int32 Slack) {
    if (!(ElementProp->PropertyFlags & CPF_NoDestructor)) {
        for (int32 Index = 0; Index < GetMaxIndex(); Index++)
            if (IsValidIndex(Index)) ElementProp->DestroyValue(GetElementPtr(Index));
    }
    Set->Empty(Slack, SetLayout);
}

int32 FScriptSetHelper::AddDefaultValue_Invalid_NeedsRehash() {
    int32 Index = Set->AddUninitialized(SetLayout);
    uint8* Element = GetElementPtr(Index);
    std::memset(Element, 0, static_cast<std::size_t>(SetLayout.Size));
    ElementProp->InitializeValue(Element);
    return Index;
}

void FScriptSetHelper::Rehash() {
    Set->ResetHash(Num());
    for (int32 Index = 0; Index < GetMaxIndex(); Index++)
        if (IsValidIndex(Index)) LinkElement(Index, ElementProp->GetValueTypeHash(GetElementPtr(Index)));
}

FScriptMapHelper FScriptMapHelper::CreateHelperFormInnerProperties(const FProperty* InKeyProp, const FProperty* InValueProp,
                                                                   const void* InMap) {
    FScriptMapLayout Layout = GetScriptMapLayout(InKeyProp->GetSize(), InKeyProp->GetMinAlignment(), InValueProp->GetSize(),
                                                 InValueProp->GetMinAlignment());
    return FScriptMapHelper(InKeyProp, InValueProp, Layout, InMap);
}

int32 FScriptMapHelper::FindInternalIndex(int32 LogicalIndex) const {
    if (LogicalIndex < 0 || LogicalIndex >= Num()) return INDEX_NONE;
    for (int32 Index = 0; Index < GetMaxIndex(); Index++) {
        if (!IsValidIndex(Index)) continue;
        if (LogicalIndex-- == 0) return Index;
    }
    return INDEX_NONE;
}

int32 FScriptMapHelper::FindMapIndexWithKey(const void* KeyPtr) const {
    return FindInHash(Map->Pairs, MapLayout.SetLayout, KeyProp, KeyPtr);
}

uint8* FScriptMapHelper::FindValueFromHash(const void* KeyPtr) {
    int32 Index = FindMapIndexWithKey(KeyPtr);
    return Index == INDEX_NONE ? nullptr : GetValuePtr(Index);
}

void FScriptMapHelper::LinkPair(int32 Index, uint32 Hash) { LinkIntoHash(Map->Pairs, MapLayout.SetLayout, Index, Hash); }

void FScriptMapHelper::AddPair(const void* KeyPtr, const void* ValuePtr) {
    if (uint8* Existing = FindValueFromHash(KeyPtr)) {
        ValueProp->CopySingleValue(Existing, ValuePtr);
        return;
    }
    int32 Index = AddDefaultValue_Invalid_NeedsRehash();
    KeyProp->CopySingleValue(GetKeyPtr(Index), KeyPtr);
    ValueProp->CopySingleValue(GetValuePtr(Index), ValuePtr);
    LinkOrRehash(*this, Map->Pairs, MapLayout.SetLayout, Index, KeyProp->GetValueTypeHash(GetKeyPtr(Index)));
}

uint8* FScriptMapHelper::FindOrAdd(const void* KeyPtr) {
    if (uint8* Existing = FindValueFromHash(KeyPtr)) return Existing;
    int32 Index = AddDefaultValue_Invalid_NeedsRehash();
    KeyProp->CopySingleValue(GetKeyPtr(Index), KeyPtr);
    LinkOrRehash(*this, Map->Pairs, MapLayout.SetLayout, Index, KeyProp->GetValueTypeHash(GetKeyPtr(Index)));
    return GetValuePtr(Index);
}

void FScriptMapHelper::RemoveAt(int32 Index, int32 Count) {
    for (; Count > 0 && Index < GetMaxIndex(); Index++) {
        if (!IsValidIndex(Index)) continue;
        KeyProp->DestroyValue(GetKeyPtr(Index));
        ValueProp->DestroyValue(GetValuePtr(Index));
        Map->Pairs.RemoveAt(Index, MapLayout.SetLayout);
        Count--;
    }
}

bool FScriptMapHelper::RemovePair(const void* KeyPtr) {
    int32 Index = FindMapIndexWithKey(KeyPtr);
    if (Index == INDEX_NONE) return false;
    RemoveAt(Index);
    return true;
}

void FScriptMapHelper::EmptyValues(int32 Slack) {
    bool bDestroy = !(KeyProp->PropertyFlags & CPF_NoDestructor) || !(ValueProp->PropertyFlags & CPF_NoDestructor);
    if (bDestroy) {
        for (int32 Index = 0; Index < GetMaxIndex(); Index++) {
            if (!IsValidIndex(Index)) continue;
            KeyProp->DestroyValue(GetKeyPtr(Index));
            ValueProp->DestroyValue(GetValuePtr(Index));
        }
    }
    Map->Pairs.Empty(Slack, MapLayout.SetLayout);
}

int32 FScriptMapHelper::AddDefaultValue_Invalid_NeedsRehash() {
    int32 Index = Map->Pairs.AddUninitialized(MapLayout.SetLayout);
    uint8* Pair = GetPairPtr(Index);
    std::memset(Pair, 0, static_cast<std::size_t>(MapLayout.SetLayout.Size));
    KeyProp->InitializeValue(Pair);
    ValueProp->InitializeValue(Pair + MapLayout.ValueOffset);
    return Index;
}

void FScriptMapHelper::Rehash() {
    Map->Pairs.ResetHash(Num());
    for (int32 Index = 0; Index < GetMaxIndex(); Index++)
        if (IsValidIndex(Index)) LinkPair(Index, KeyProp->GetValueTypeHash(GetKeyPtr(Index)));
}

}
