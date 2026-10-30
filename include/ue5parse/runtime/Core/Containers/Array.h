#pragma once

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <cstring>
#include <initializer_list>
#include <new>
#include <span>
#include <type_traits>
#include <utility>

#include "ue5parse/core/Check.h"

namespace ue5parse::runtime {

using int8 = std::int8_t;
using int16 = std::int16_t;
using int32 = std::int32_t;
using int64 = std::int64_t;
using uint8 = std::uint8_t;
using uint16 = std::uint16_t;
using uint32 = std::uint32_t;
using uint64 = std::uint64_t;
using TCHAR = char16_t;

constexpr int32 INDEX_NONE = -1;

// Memory of the runtime containers and objects. Aligned to at least 16 bytes, as FMemory::Malloc.
void* RuntimeMalloc(std::size_t size, std::size_t alignment = 16);
void* RuntimeRealloc(void* memory, std::size_t oldSize, std::size_t newSize, std::size_t alignment = 16);
void RuntimeFree(void* memory);

// The untyped dynamic array of the script VM and the properties: a pointer, a count and a capacity. Elements move by
// memcpy, as in UE: every type a container holds must be bitwise relocatable.
class FScriptArray {
public:
    FScriptArray() = default;
    FScriptArray(const FScriptArray&) = delete;
    FScriptArray& operator=(const FScriptArray&) = delete;

    void* GetData() { return Data; }
    const void* GetData() const { return Data; }
    int32 Num() const { return ArrayNum; }
    int32 Max() const { return ArrayMax; }
    bool IsValidIndex(int32 Index) const { return Index >= 0 && Index < ArrayNum; }

    // Adds Count uninitialized elements at the end and returns the index of the first.
    int32 Add(int32 Count, int32 ElementSize, int32 Alignment);
    // Adds Count zeroed elements at the end.
    int32 AddZeroed(int32 Count, int32 ElementSize, int32 Alignment);
    void Insert(int32 Index, int32 Count, int32 ElementSize, int32 Alignment);
    // Removes elements without destructing them; the elements after them move down.
    void Remove(int32 Index, int32 Count, int32 ElementSize, int32 Alignment);
    // Sets the count to 0 and the capacity to Slack (frees the memory for 0).
    void Empty(int32 Slack, int32 ElementSize, int32 Alignment);
    void Reserve(int32 Capacity, int32 ElementSize, int32 Alignment);
    // Sets the count without constructing or destructing; for the helpers that do it.
    void SetNumUninitialized(int32 NewNum, int32 ElementSize, int32 Alignment);
    void SwapMemory(int32 A, int32 B, int32 ElementSize);
    void MoveAssign(FScriptArray& Other);

protected:
    void ResizeTo(int32 NewMax, int32 ElementSize, int32 Alignment);

    void* Data = nullptr;
    int32 ArrayNum = 0;
    int32 ArrayMax = 0;
};

// TArray: the typed view of FScriptArray, with the same layout, so that a C++ member can be an FArrayProperty's value.
template <class T>
class TArray {
public:
    TArray() = default;
    TArray(std::initializer_list<T> Items) {
        Reserve(static_cast<int32>(Items.size()));
        for (const T& Item : Items) Add(Item);
    }
    TArray(const TArray& Other) { *this = Other; }
    TArray(TArray&& Other) noexcept { Array.MoveAssign(Other.Array); }
    ~TArray() { Empty(); }

    TArray& operator=(const TArray& Other) {
        if (this == &Other) return *this;
        Empty(Other.Num());
        for (const T& Item : Other) Add(Item);
        return *this;
    }
    TArray& operator=(TArray&& Other) noexcept {
        if (this == &Other) return *this;
        Empty();
        Array.MoveAssign(Other.Array);
        return *this;
    }

    int32 Num() const { return Array.Num(); }
    bool IsEmpty() const { return Array.Num() == 0; }
    bool IsValidIndex(int32 Index) const { return Array.IsValidIndex(Index); }
    T* GetData() { return static_cast<T*>(Array.GetData()); }
    const T* GetData() const { return static_cast<const T*>(Array.GetData()); }
    // The elements as a view (UE's implicit TArrayView conversion).
    operator std::span<T>() { return std::span<T>(GetData(), static_cast<std::size_t>(Num())); }
    operator std::span<const T>() const { return std::span<const T>(GetData(), static_cast<std::size_t>(Num())); }
    T& operator[](int32 Index) {
        UE5PARSE_CHECK(IsValidIndex(Index));
        return GetData()[Index];
    }
    const T& operator[](int32 Index) const {
        UE5PARSE_CHECK(IsValidIndex(Index));
        return GetData()[Index];
    }
    T& Last() { return (*this)[Num() - 1]; }
    const T& Last() const { return (*this)[Num() - 1]; }
    T* begin() { return GetData(); }
    T* end() { return GetData() + Num(); }
    const T* begin() const { return GetData(); }
    const T* end() const { return GetData() + Num(); }

    int32 Add(const T& Item) {
        // Item can be an element of this array: copy before the memory moves.
        T Copy(Item);
        int32 Index = Array.Add(1, sizeof(T), alignof(T));
        new (GetData() + Index) T(std::move(Copy));
        return Index;
    }
    int32 Add(T&& Item) {
        T Moved(std::move(Item));
        int32 Index = Array.Add(1, sizeof(T), alignof(T));
        new (GetData() + Index) T(std::move(Moved));
        return Index;
    }
    // Constructs an element at the end from the arguments, which must not refer into the array.
    template <class... ArgsType>
    T& Emplace_GetRef(ArgsType&&... Args) {
        int32 Index = Array.Add(1, sizeof(T), alignof(T));
        return *new (GetData() + Index) T(std::forward<ArgsType>(Args)...);
    }
    // Adds elements without constructing them (trivial types only).
    int32 AddUninitialized(int32 Count = 1) {
        static_assert(std::is_trivially_copyable_v<T>);
        return Array.Add(Count, sizeof(T), alignof(T));
    }
    // Elements of zeroed memory (UE's AddZeroed), for the types whose zero bytes are a valid value.
    int32 AddZeroed(int32 Count = 1) {
        int32 Index = Array.Add(Count, sizeof(T), alignof(T));
        std::memset(static_cast<void*>(GetData() + Index), 0, sizeof(T) * static_cast<std::size_t>(Count));
        return Index;
    }
    int32 AddDefaulted(int32 Count = 1) {
        int32 Index = Array.Add(Count, sizeof(T), alignof(T));
        for (int32 i = 0; i < Count; i++) new (GetData() + Index + i) T();
        return Index;
    }
    int32 AddUnique(const T& Item) {
        int32 Index = Find(Item);
        return Index != INDEX_NONE ? Index : Add(Item);
    }
    void Insert(const T& Item, int32 Index) {
        T Copy(Item);
        Array.Insert(Index, 1, sizeof(T), alignof(T));
        new (GetData() + Index) T(std::move(Copy));
    }
    void RemoveAt(int32 Index, int32 Count = 1) {
        for (int32 i = 0; i < Count; i++) GetData()[Index + i].~T();
        Array.Remove(Index, Count, sizeof(T), alignof(T));
    }
    // Removes elements and moves the last ones into the hole: the order changes, as UE's RemoveAtSwap.
    void RemoveAtSwap(int32 Index, int32 Count = 1) {
        UE5PARSE_CHECK(Index >= 0 && Count >= 0 && Index + Count <= Num());
        for (int32 i = 0; i < Count; i++) GetData()[Index + i].~T();
        int32 NumElementsAfterHole = Num() - (Index + Count);
        int32 NumElementsToMove = NumElementsAfterHole < Count ? NumElementsAfterHole : Count;
        if (NumElementsToMove > 0)
            std::memcpy(static_cast<void*>(GetData() + Index), GetData() + (Num() - NumElementsToMove), sizeof(T) * NumElementsToMove);
        Array.SetNumUninitialized(Num() - Count, sizeof(T), alignof(T));
    }
    void RemoveSingleSwap(const T& Item) {
        int32 Index = Find(Item);
        if (Index != INDEX_NONE) RemoveAtSwap(Index);
    }
    template <class Predicate>
    int32 RemoveAllSwap(Predicate Pred) {
        int32 Removed = 0;
        for (int32 i = 0; i < Num();) {
            if (Pred(GetData()[i])) {
                RemoveAtSwap(i);
                Removed++;
            } else {
                i++;
            }
        }
        return Removed;
    }
    template <class Predicate>
    void Sort(Predicate Pred) {
        std::sort(GetData(), GetData() + Num(), Pred);
    }
    void Sort() { std::sort(GetData(), GetData() + Num()); }
    template <class Predicate>
    void StableSort(Predicate Pred) {
        std::stable_sort(GetData(), GetData() + Num(), Pred);
    }
    // Removes the elements for which the predicate is true and keeps the order of the others.
    template <class Predicate>
    int32 RemoveAll(Predicate Pred) {
        int32 Removed = 0;
        for (int32 i = Num() - 1; i >= 0; i--)
            if (Pred(GetData()[i])) {
                RemoveAt(i);
                Removed++;
            }
        return Removed;
    }
    template <class Predicate>
    int32 IndexOfByPredicate(Predicate Pred) const {
        for (int32 i = 0; i < Num(); i++)
            if (Pred(GetData()[i])) return i;
        return INDEX_NONE;
    }
    // Removes every element equal to Item and returns how many.
    int32 Remove(const T& Item) {
        int32 Removed = 0;
        for (int32 i = Num() - 1; i >= 0; i--)
            if (GetData()[i] == Item) {
                RemoveAt(i);
                Removed++;
            }
        return Removed;
    }
    int32 Find(const T& Item) const {
        for (int32 i = 0; i < Num(); i++)
            if (GetData()[i] == Item) return i;
        return INDEX_NONE;
    }
    bool Contains(const T& Item) const { return Find(Item) != INDEX_NONE; }
    void Empty(int32 Slack = 0) {
        for (T& Item : *this) Item.~T();
        Array.Empty(Slack, sizeof(T), alignof(T));
    }
    // Removes the elements and keeps room for NewSize.
    void Reset(int32 NewSize = 0) {
        for (T& Item : *this) Item.~T();
        Array.SetNumUninitialized(0, sizeof(T), alignof(T));
        if (NewSize > 0) Reserve(NewSize);
    }
    void Reserve(int32 Capacity) { Array.Reserve(Capacity, sizeof(T), alignof(T)); }
    void SetNum(int32 NewNum) {
        if (NewNum > Num()) {
            AddDefaulted(NewNum - Num());
        } else if (NewNum < Num()) {
            RemoveAt(NewNum, Num() - NewNum);
        }
    }
    T Pop() {
        T Item(std::move(Last()));
        RemoveAt(Num() - 1);
        return Item;
    }
    void Push(const T& Item) { Add(Item); }

    bool operator==(const TArray& Other) const {
        if (Num() != Other.Num()) return false;
        for (int32 i = 0; i < Num(); i++)
            if (!(GetData()[i] == Other.GetData()[i])) return false;
        return true;
    }

private:
    FScriptArray Array;
};

static_assert(sizeof(TArray<int32>) == sizeof(FScriptArray));

}
