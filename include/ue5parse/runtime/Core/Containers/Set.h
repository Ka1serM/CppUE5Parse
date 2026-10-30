#pragma once

#include <utility>

#include "ue5parse/runtime/Core/Containers/Array.h"
#include "ue5parse/runtime/Core/Templates/TypeHash.h"

namespace ue5parse::runtime {

// TSet: unique elements in the order they were added, found by GetTypeHash and operator== (UE's Containers/Set.h).
// Removing keeps the order of the others. A C++ set is not a reflected value: reflected sets are FScriptSet memory.
template <class T>
class TSet {
public:
    TSet() = default;
    TSet(std::initializer_list<T> Items) {
        for (const T& Item : Items) Add(Item);
    }

    int32 Num() const { return Elements.Num(); }
    bool IsEmpty() const { return Elements.IsEmpty(); }
    // Adds the element unless an equal one is in the set; the element of the set either way.
    T& Add(const T& Item) {
        if (T* Existing = Find(Item)) return *Existing;
        int32 Index = Elements.Add(Item);
        Link(Index);
        return Elements[Index];
    }
    bool Contains(const T& Item) const { return FindIndex(Item) != INDEX_NONE; }
    T* Find(const T& Item) {
        int32 Index = FindIndex(Item);
        return Index != INDEX_NONE ? &Elements[Index] : nullptr;
    }
    const T* Find(const T& Item) const {
        int32 Index = FindIndex(Item);
        return Index != INDEX_NONE ? &Elements[Index] : nullptr;
    }
    // The number of elements removed: 0 or 1.
    int32 Remove(const T& Item) {
        int32 Index = FindIndex(Item);
        if (Index == INDEX_NONE) return 0;
        Elements.RemoveAt(Index);
        Rehash();
        return 1;
    }
    void Empty() {
        Elements.Empty();
        Buckets.Empty();
        NextInBucket.Empty();
    }
    void Reset() { Empty(); }
    TArray<T> Array() const { return Elements; }
    const T* begin() const { return Elements.begin(); }
    const T* end() const { return Elements.end(); }
    T* begin() { return Elements.begin(); }
    T* end() { return Elements.end(); }

private:
    int32 FindIndex(const T& Item) const {
        if (Buckets.IsEmpty()) return INDEX_NONE;
        uint32 Hash = GetTypeHash(Item);
        for (int32 Index = Buckets[static_cast<int32>(Hash & static_cast<uint32>(Buckets.Num() - 1))]; Index != INDEX_NONE;
             Index = NextInBucket[Index])
            if (Elements[Index] == Item) return Index;
        return INDEX_NONE;
    }
    void Link(int32 Index) {
        if (Elements.Num() > Buckets.Num()) {
            Rehash();
            return;
        }
        NextInBucket.Add(INDEX_NONE);
        int32& Head = Buckets[static_cast<int32>(GetTypeHash(Elements[Index]) & static_cast<uint32>(Buckets.Num() - 1))];
        NextInBucket[Index] = Head;
        Head = Index;
    }
    void Rehash() {
        int32 Count = 8;
        while (Count < Elements.Num()) Count *= 2;
        Buckets.Empty(Count);
        for (int32 i = 0; i < Count; i++) Buckets.Add(INDEX_NONE);
        NextInBucket.Empty(Elements.Num());
        for (int32 Index = 0; Index < Elements.Num(); Index++) {
            NextInBucket.Add(INDEX_NONE);
            int32& Head = Buckets[static_cast<int32>(GetTypeHash(Elements[Index]) & static_cast<uint32>(Count - 1))];
            NextInBucket[Index] = Head;
            Head = Index;
        }
    }

    TArray<T> Elements;
    TArray<int32> Buckets;
    TArray<int32> NextInBucket;
};

}
