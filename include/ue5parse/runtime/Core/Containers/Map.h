#pragma once

#include <utility>

#include "ue5parse/runtime/Core/Containers/Array.h"
#include "ue5parse/runtime/Core/Templates/TypeHash.h"

namespace ue5parse::runtime {

template <class KeyType, class ValueType>
struct TPair {
    KeyType Key;
    ValueType Value;
    bool operator==(const TPair& Other) const { return Key == Other.Key && Value == Other.Value; }
};

// TMap: key-value pairs in the order they were added, found by the key's GetTypeHash and operator== (UE's
// Containers/Map.h). Removing keeps the order of the others. A C++ map is not a reflected value: reflected maps are
// FScriptMap memory, read through FScriptMapHelper.
template <class KeyType, class ValueType>
class TMap {
public:
    using ElementType = TPair<KeyType, ValueType>;

    int32 Num() const { return Pairs.Num(); }
    bool IsEmpty() const { return Pairs.IsEmpty(); }
    // Sets the value of a key, adding the pair when the key is new.
    ValueType& Add(const KeyType& Key, const ValueType& Value) {
        if (ValueType* Existing = Find(Key)) {
            *Existing = Value;
            return *Existing;
        }
        int32 Index = Pairs.Add(ElementType{Key, Value});
        Link(Index);
        return Pairs[Index].Value;
    }
    ValueType& Add(const KeyType& Key) { return FindOrAdd(Key); }
    ValueType& FindOrAdd(const KeyType& Key) {
        if (ValueType* Existing = Find(Key)) return *Existing;
        int32 Index = Pairs.Add(ElementType{Key, ValueType()});
        Link(Index);
        return Pairs[Index].Value;
    }
    ValueType* Find(const KeyType& Key) {
        int32 Index = FindIndex(Key);
        return Index != INDEX_NONE ? &Pairs[Index].Value : nullptr;
    }
    const ValueType* Find(const KeyType& Key) const {
        int32 Index = FindIndex(Key);
        return Index != INDEX_NONE ? &Pairs[Index].Value : nullptr;
    }
    ValueType FindRef(const KeyType& Key) const {
        const ValueType* Value = Find(Key);
        return Value ? *Value : ValueType();
    }
    ValueType& FindChecked(const KeyType& Key) {
        ValueType* Value = Find(Key);
        UE5PARSE_CHECK(Value != nullptr);
        return *Value;
    }
    const ValueType& FindChecked(const KeyType& Key) const {
        const ValueType* Value = Find(Key);
        UE5PARSE_CHECK(Value != nullptr);
        return *Value;
    }
    bool Contains(const KeyType& Key) const { return FindIndex(Key) != INDEX_NONE; }
    // The number of pairs removed: 0 or 1.
    int32 Remove(const KeyType& Key) {
        int32 Index = FindIndex(Key);
        if (Index == INDEX_NONE) return 0;
        Pairs.RemoveAt(Index);
        Rehash();
        return 1;
    }
    bool RemoveAndCopyValue(const KeyType& Key, ValueType& OutValue) {
        int32 Index = FindIndex(Key);
        if (Index == INDEX_NONE) return false;
        OutValue = std::move(Pairs[Index].Value);
        Pairs.RemoveAt(Index);
        Rehash();
        return true;
    }
    void Empty() {
        Pairs.Empty();
        Buckets.Empty();
        NextInBucket.Empty();
    }
    void Reset() { Empty(); }
    void GenerateKeyArray(TArray<KeyType>& OutKeys) const {
        OutKeys.Empty(Pairs.Num());
        for (const ElementType& Pair : Pairs) OutKeys.Add(Pair.Key);
    }
    void GenerateValueArray(TArray<ValueType>& OutValues) const {
        OutValues.Empty(Pairs.Num());
        for (const ElementType& Pair : Pairs) OutValues.Add(Pair.Value);
    }
    ElementType* begin() { return Pairs.begin(); }
    ElementType* end() { return Pairs.end(); }
    const ElementType* begin() const { return Pairs.begin(); }
    const ElementType* end() const { return Pairs.end(); }
    ValueType& operator[](const KeyType& Key) { return FindChecked(Key); }
    const ValueType& operator[](const KeyType& Key) const { return FindChecked(Key); }

private:
    int32 FindIndex(const KeyType& Key) const {
        if (Buckets.IsEmpty()) return INDEX_NONE;
        uint32 Hash = GetTypeHash(Key);
        for (int32 Index = Buckets[static_cast<int32>(Hash & static_cast<uint32>(Buckets.Num() - 1))]; Index != INDEX_NONE;
             Index = NextInBucket[Index])
            if (Pairs[Index].Key == Key) return Index;
        return INDEX_NONE;
    }
    void Link(int32 Index) {
        if (Pairs.Num() > Buckets.Num()) {
            Rehash();
            return;
        }
        NextInBucket.Add(INDEX_NONE);
        int32& Head = Buckets[static_cast<int32>(GetTypeHash(Pairs[Index].Key) & static_cast<uint32>(Buckets.Num() - 1))];
        NextInBucket[Index] = Head;
        Head = Index;
    }
    void Rehash() {
        int32 Count = 8;
        while (Count < Pairs.Num()) Count *= 2;
        Buckets.Empty(Count);
        for (int32 i = 0; i < Count; i++) Buckets.Add(INDEX_NONE);
        NextInBucket.Empty(Pairs.Num());
        for (int32 Index = 0; Index < Pairs.Num(); Index++) {
            NextInBucket.Add(INDEX_NONE);
            int32& Head = Buckets[static_cast<int32>(GetTypeHash(Pairs[Index].Key) & static_cast<uint32>(Count - 1))];
            NextInBucket[Index] = Head;
            Head = Index;
        }
    }

    TArray<ElementType> Pairs;
    TArray<int32> Buckets;
    TArray<int32> NextInBucket;
};

}
