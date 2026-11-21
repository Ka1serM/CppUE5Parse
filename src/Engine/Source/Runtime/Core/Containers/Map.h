#pragma once

#include <cstddef>
#include <initializer_list>
#include <utility>

#include "Engine/Source/Runtime/Core/Containers/Set.h"

namespace ue5parse::runtime {

template <class KeyType, class ValueType>
struct TPair {
    KeyType Key;
    ValueType Value;
    bool operator==(const TPair& Other) const { return Key == Other.Key && Value == Other.Value; }
};

template <class KeyType, class ValueType>
uint32 GetTypeHash(const TPair<KeyType, ValueType>& Pair) {
    return HashCombine(GetTypeHash(Pair.Key), GetTypeHash(Pair.Value));
}

template <class InKeyType, class InValueType>
struct TDefaultMapKeyFuncs {
    using KeyType = InKeyType;
    using ElementType = TPair<InKeyType, InValueType>;
    static const KeyType& GetSetKey(const ElementType& Element) { return Element.Key; }
    static bool Matches(const KeyType& A, const KeyType& B) { return A == B; }
    static uint32 GetKeyHash(const KeyType& Key) { return GetTypeHash(Key); }
};

//! The memory is an FScriptMap's, so a TMap UPROPERTY is read and written by FMapProperty.
template <class KeyType, class ValueType>
class TMap {
public:
    using ElementType = TPair<KeyType, ValueType>;

private:
    using SetType = TSet<ElementType, TDefaultMapKeyFuncs<KeyType, ValueType>>;

    static constexpr FScriptMapLayout Layout = GetScriptMapLayout(sizeof(KeyType), alignof(KeyType), sizeof(ValueType), alignof(ValueType));
    static_assert(offsetof(ElementType, Value) == Layout.ValueOffset);
    static_assert(GetScriptSetLayout(sizeof(ElementType), alignof(ElementType)).Size == Layout.SetLayout.Size);

public:
    TMap() = default;
    TMap(std::initializer_list<ElementType> Items) {
        for (const ElementType& Item : Items)
            Add(Item.Key, Item.Value);
    }

    int32 Num() const { return Pairs.Num(); }
    bool IsEmpty() const { return Pairs.IsEmpty(); }

    ValueType& Add(const KeyType& Key, const ValueType& Value) { return Pairs.Add(ElementType{Key, Value}).Value; }
    ValueType& Add(const KeyType& Key, ValueType&& Value) { return Pairs.Add(ElementType{Key, std::move(Value)}).Value; }
    ValueType& Add(const KeyType& Key) { return FindOrAdd(Key); }
    template <class... ArgTypes>
    ValueType& Emplace(const KeyType& Key, ArgTypes&&... Args) {
        return Pairs.Emplace(ElementType{Key, ValueType(std::forward<ArgTypes>(Args)...)}).Value;
    }
    ValueType& FindOrAdd(const KeyType& Key) {
        if (ValueType* Existing = Find(Key))
            return *Existing;
        return Pairs.Add(ElementType{Key, ValueType()}).Value;
    }
    void Append(const TMap& Other) {
        for (const ElementType& Pair : Other)
            Add(Pair.Key, Pair.Value);
    }
    void Reserve(int32 Number) { Pairs.Reserve(Number); }

    ValueType* Find(const KeyType& Key) {
        ElementType* Pair = Pairs.Find(Key);
        return Pair != nullptr ? &Pair->Value : nullptr;
    }
    const ValueType* Find(const KeyType& Key) const {
        const ElementType* Pair = Pairs.Find(Key);
        return Pair != nullptr ? &Pair->Value : nullptr;
    }
    ValueType FindRef(const KeyType& Key) const {
        const ValueType* Value = Find(Key);
        return Value != nullptr ? *Value : ValueType();
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
    const KeyType* FindKey(const ValueType& Value) const {
        for (const ElementType& Pair : Pairs)
            if (Pair.Value == Value)
                return &Pair.Key;
        return nullptr;
    }
    bool Contains(const KeyType& Key) const { return Pairs.Contains(Key); }
    ValueType& operator[](const KeyType& Key) { return FindChecked(Key); }
    const ValueType& operator[](const KeyType& Key) const { return FindChecked(Key); }

    int32 Remove(const KeyType& Key) { return Pairs.Remove(Key); }
    bool RemoveAndCopyValue(const KeyType& Key, ValueType& OutValue) {
        const int32 Id = Pairs.FindId(Key);
        if (Id == INDEX_NONE)
            return false;
        OutValue = std::move(Pairs[Id].Value);
        Pairs.RemoveId(Id);
        return true;
    }
    void Empty(int32 Slack = 0) { Pairs.Empty(Slack); }
    void Reset() { Pairs.Reset(); }

    void GenerateKeyArray(TArray<KeyType>& OutKeys) const {
        OutKeys.Reset();
        for (const ElementType& Pair : Pairs)
            OutKeys.Add(Pair.Key);
    }
    void GenerateValueArray(TArray<ValueType>& OutValues) const {
        OutValues.Reset();
        for (const ElementType& Pair : Pairs)
            OutValues.Add(Pair.Value);
    }

    typename SetType::Iterator CreateIterator() { return Pairs.CreateIterator(); }
    typename SetType::ConstIterator CreateConstIterator() const { return Pairs.CreateConstIterator(); }
    typename SetType::Iterator begin() { return Pairs.begin(); }
    typename SetType::Iterator end() { return Pairs.end(); }
    typename SetType::ConstIterator begin() const { return Pairs.begin(); }
    typename SetType::ConstIterator end() const { return Pairs.end(); }

private:
    SetType Pairs;
};

}
