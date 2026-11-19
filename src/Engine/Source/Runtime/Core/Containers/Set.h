#pragma once

#include <cstddef>
#include <initializer_list>
#include <new>
#include <utility>

#include "Engine/Source/Runtime/Core/Containers/Array.h"
#include "Engine/Source/Runtime/Core/Containers/ScriptSet.h"
#include "Engine/Source/Runtime/Core/Templates/TypeHash.h"

namespace ue5parse::runtime {

template <class ElementType>
struct DefaultKeyFuncs {
    using KeyType = ElementType;
    static const KeyType& GetSetKey(const ElementType& Element) { return Element; }
    static bool Matches(const KeyType& A, const KeyType& B) { return A == B; }
    static uint32 GetKeyHash(const KeyType& Key) { return GetTypeHash(Key); }
};

//! The memory is an FScriptSet's, so a TSet UPROPERTY is read and written by FSetProperty.
template <class ElementType, class KeyFuncs = DefaultKeyFuncs<ElementType>>
class TSet {
    using KeyType = typename KeyFuncs::KeyType;

    struct FElement {
        ElementType Value;
        int32 HashNextId;
        int32 HashIndex;
    };

    static constexpr FScriptSetLayout Layout = GetScriptSetLayout(sizeof(ElementType), alignof(ElementType));
    static_assert(offsetof(FElement, HashNextId) == Layout.HashNextIdOffset && offsetof(FElement, HashIndex) == Layout.HashIndexOffset);
    static_assert(sizeof(FElement) == Layout.Size && alignof(FElement) <= Layout.Alignment);

public:
    template <class SetType, class ValueType>
    class TIterator {
    public:
        TIterator(SetType* InSet, int32 InIndex) : Set(InSet), Index(InIndex) { Skip(); }
        ValueType& operator*() const { return Set->Element(Index).Value; }
        ValueType* operator->() const { return &Set->Element(Index).Value; }
        TIterator& operator++() {
            Index++;
            Skip();
            return *this;
        }
        bool operator==(const TIterator& Other) const { return Index == Other.Index; }
        explicit operator bool() const { return Index < Set->Storage.GetMaxIndex(); }
        int32 GetId() const { return Index; }

    private:
        void Skip() {
            while (Index < Set->Storage.GetMaxIndex() && !Set->Storage.IsValidIndex(Index))
                Index++;
        }

        SetType* Set;
        int32 Index;
    };
    using Iterator = TIterator<TSet, ElementType>;
    using ConstIterator = TIterator<const TSet, const ElementType>;

    TSet() = default;
    TSet(std::initializer_list<ElementType> Items) { Append(Items); }
    TSet(const TSet& Other) { Append(Other); }
    TSet(TSet&& Other) noexcept { Storage.MoveAssign(Other.Storage, Layout); }
    TSet& operator=(const TSet& Other) {
        if (this == &Other)
            return *this;
        Empty();
        Append(Other);
        return *this;
    }
    TSet& operator=(TSet&& Other) noexcept {
        if (this == &Other)
            return *this;
        DestructElements();
        Storage.MoveAssign(Other.Storage, Layout);
        return *this;
    }
    ~TSet() { DestructElements(); }

    int32 Num() const { return Storage.Num(); }
    bool IsEmpty() const { return Num() == 0; }
    int32 GetMaxIndex() const { return Storage.GetMaxIndex(); }
    bool IsValidId(int32 Id) const { return Storage.IsValidIndex(Id); }

    ElementType& Add(const ElementType& Item) { return Emplace(Item); }
    ElementType& Add(ElementType&& Item) { return Emplace(std::move(Item)); }
    template <class... ArgTypes>
    ElementType& Emplace(ArgTypes&&... Args) {
        const int32 Index = Storage.AddUninitialized(Layout);
        FElement& New = Element(Index);
        new (&New.Value) ElementType(std::forward<ArgTypes>(Args)...);
        const int32 Existing = FindId(KeyFuncs::GetSetKey(New.Value));
        if (Existing != INDEX_NONE) {
            Element(Existing).Value = std::move(New.Value);
            New.Value.~ElementType();
            Storage.RemoveAt(Index, Layout);
            return Element(Existing).Value;
        }
        LinkOrRehash(Index);
        return Element(Index).Value;
    }
    template <class RangeType>
    void Append(const RangeType& Items) {
        for (const ElementType& Item : Items)
            Add(Item);
    }
    void Reserve(int32 Number) {
        if (Number > Storage.GetHashSize())
            Rehash(Number);
    }

    ElementType* Find(const KeyType& Key) {
        const int32 Id = FindId(Key);
        return Id != INDEX_NONE ? &Element(Id).Value : nullptr;
    }
    const ElementType* Find(const KeyType& Key) const {
        const int32 Id = FindId(Key);
        return Id != INDEX_NONE ? &Element(Id).Value : nullptr;
    }
    bool Contains(const KeyType& Key) const { return FindId(Key) != INDEX_NONE; }
    int32 FindId(const KeyType& Key) const {
        if (Num() == 0 || Storage.GetHashSize() == 0)
            return INDEX_NONE;
        for (int32 Id = Storage.GetHashBucket(KeyFuncs::GetKeyHash(Key)); Id != INDEX_NONE; Id = Element(Id).HashNextId)
            if (KeyFuncs::Matches(KeyFuncs::GetSetKey(Element(Id).Value), Key))
                return Id;
        return INDEX_NONE;
    }
    ElementType& operator[](int32 Id) { return Element(Id).Value; }
    const ElementType& operator[](int32 Id) const { return Element(Id).Value; }

    int32 Remove(const KeyType& Key) {
        const int32 Id = FindId(Key);
        if (Id == INDEX_NONE)
            return 0;
        RemoveId(Id);
        return 1;
    }
    void RemoveId(int32 Id) {
        Element(Id).Value.~ElementType();
        Storage.RemoveAt(Id, Layout);
    }
    void Empty(int32 Slack = 0) {
        DestructElements();
        Storage.Empty(Slack, Layout);
    }
    void Reset() { Empty(); }

    TArray<ElementType> Array() const {
        TArray<ElementType> Result;
        Result.Reserve(Num());
        for (const ElementType& Item : *this)
            Result.Add(Item);
        return Result;
    }

    Iterator CreateIterator() { return Iterator(this, 0); }
    ConstIterator CreateConstIterator() const { return ConstIterator(this, 0); }
    Iterator begin() { return Iterator(this, 0); }
    Iterator end() { return Iterator(this, Storage.GetMaxIndex()); }
    ConstIterator begin() const { return ConstIterator(this, 0); }
    ConstIterator end() const { return ConstIterator(this, Storage.GetMaxIndex()); }

private:
    FElement& Element(int32 Index) { return *static_cast<FElement*>(Storage.GetData(Index, Layout)); }
    const FElement& Element(int32 Index) const { return *static_cast<const FElement*>(Storage.GetData(Index, Layout)); }

    void Link(int32 Index) {
        FElement& Item = Element(Index);
        const uint32 Hash = KeyFuncs::GetKeyHash(KeyFuncs::GetSetKey(Item.Value));
        Item.HashIndex = static_cast<int32>(Hash);
        int32& Bucket = Storage.GetHashBucket(Hash);
        Item.HashNextId = Bucket;
        Bucket = Index;
    }
    void LinkOrRehash(int32 Index) {
        if (Storage.GetHashSize() < FScriptSet::GetNumberOfHashBuckets(Num()))
            Rehash(Num());
        else
            Link(Index);
    }
    void Rehash(int32 Number) {
        Storage.ResetHash(Number);
        for (int32 Index = 0; Index < Storage.GetMaxIndex(); Index++)
            if (Storage.IsValidIndex(Index))
                Link(Index);
    }
    void DestructElements() {
        for (int32 Index = 0; Index < Storage.GetMaxIndex(); Index++)
            if (Storage.IsValidIndex(Index))
                Element(Index).Value.~ElementType();
    }

    FScriptSet Storage;
};

}
