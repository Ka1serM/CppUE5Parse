#pragma once

#include "ue5parse/runtime/Core/Containers/Array.h"

namespace ue5parse::runtime {

// The memory of TSet and TMap values in script memory, with UE's ordering rules: elements live in a sparse array, a
// removed element leaves a hole, and a new element takes the most recently freed hole first. Iteration goes by index
// over the allocated elements, so it sees the same order as UE.
//
// Each element is [value][hash next index][hash bucket]; a free element holds the free list links instead. The hash
// is rebuilt by the owner (FScriptSetHelper) when the bucket count changes.
struct FScriptSetLayout {
    int32 HashNextIdOffset = 0;
    int32 HashIndexOffset = 0;
    int32 Size = 0;
    int32 Alignment = 1;
};

FScriptSetLayout GetScriptSetLayout(int32 ElementSize, int32 ElementAlignment);

class FScriptSet {
public:
    FScriptSet() = default;
    FScriptSet(const FScriptSet&) = delete;
    FScriptSet& operator=(const FScriptSet&) = delete;

    // The number of allocated elements.
    int32 Num() const { return Elements.Num() - NumFreeIndices; }
    // One past the highest index ever allocated since the last Empty.
    int32 GetMaxIndex() const { return Elements.Num(); }
    bool IsValidIndex(int32 Index) const;
    void* GetData(int32 Index, const FScriptSetLayout& Layout) {
        return static_cast<uint8*>(Elements.GetData()) + static_cast<std::size_t>(Index) * Layout.Size;
    }
    const void* GetData(int32 Index, const FScriptSetLayout& Layout) const {
        return static_cast<const uint8*>(Elements.GetData()) + static_cast<std::size_t>(Index) * Layout.Size;
    }

    // Frees all elements (the caller destructs the values first) and keeps room for Slack.
    void Empty(int32 Slack, const FScriptSetLayout& Layout);
    // Allocates an element (a hole first) without constructing it, and returns its index. The hash is stale until
    // Rehash.
    int32 AddUninitialized(const FScriptSetLayout& Layout);
    // Frees an element (the caller destructs its value first) and unlinks it from its bucket.
    void RemoveAt(int32 Index, const FScriptSetLayout& Layout);

    // The hash buckets. Bucket heads are element indices or INDEX_NONE.
    int32 GetHashSize() const { return HashSize; }
    int32& GetHashBucket(uint32 Hash) { return Hash_[Hash & static_cast<uint32>(HashSize - 1)]; }
    int32 GetHashBucket(uint32 Hash) const { return Hash_[Hash & static_cast<uint32>(HashSize - 1)]; }
    // Resizes the buckets to fit the element count and clears them; the caller links every element again.
    void ResetHash(int32 NumElements);
    // The bucket count that UE uses for a number of elements.
    static int32 GetNumberOfHashBuckets(int32 NumElements);

    void MoveAssign(FScriptSet& Other, const FScriptSetLayout& Layout);
    ~FScriptSet();

private:
    bool IsAllocated(int32 Index) const;
    void SetAllocated(int32 Index, bool Allocated);

    FScriptArray Elements;
    FScriptArray AllocationFlags;   // one bit per element, in uint32 words
    int32 FirstFreeIndex = INDEX_NONE;
    int32 NumFreeIndices = 0;
    int32* Hash_ = nullptr;
    int32 HashSize = 0;
};

// TMap memory: a set of pairs [key][value].
struct FScriptMapLayout {
    int32 ValueOffset = 0;
    FScriptSetLayout SetLayout;
};

FScriptMapLayout GetScriptMapLayout(int32 KeySize, int32 KeyAlignment, int32 ValueSize, int32 ValueAlignment);

class FScriptMap {
public:
    FScriptSet Pairs;
};

}
