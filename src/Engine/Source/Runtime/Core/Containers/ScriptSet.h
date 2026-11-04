#pragma once

#include "Engine/Source/Runtime/Core/Containers/Array.h"

namespace ue5parse::runtime {

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

    int32 Num() const { return Elements.Num() - NumFreeIndices; }
    int32 GetMaxIndex() const { return Elements.Num(); }
    bool IsValidIndex(int32 Index) const;
    void* GetData(int32 Index, const FScriptSetLayout& Layout) {
        return static_cast<uint8*>(Elements.GetData()) + static_cast<std::size_t>(Index) * Layout.Size;
    }
    const void* GetData(int32 Index, const FScriptSetLayout& Layout) const {
        return static_cast<const uint8*>(Elements.GetData()) + static_cast<std::size_t>(Index) * Layout.Size;
    }

    void Empty(int32 Slack, const FScriptSetLayout& Layout);
    int32 AddUninitialized(const FScriptSetLayout& Layout);
    void RemoveAt(int32 Index, const FScriptSetLayout& Layout);

    int32 GetHashSize() const { return HashSize; }
    int32& GetHashBucket(uint32 Hash) { return Hash_[Hash & static_cast<uint32>(HashSize - 1)]; }
    int32 GetHashBucket(uint32 Hash) const { return Hash_[Hash & static_cast<uint32>(HashSize - 1)]; }
    void ResetHash(int32 NumElements);
    static int32 GetNumberOfHashBuckets(int32 NumElements);

    void MoveAssign(FScriptSet& Other, const FScriptSetLayout& Layout);
    ~FScriptSet();

private:
    bool IsAllocated(int32 Index) const;
    void SetAllocated(int32 Index, bool Allocated);

    FScriptArray Elements;
    FScriptArray AllocationFlags;
    int32 FirstFreeIndex = INDEX_NONE;
    int32 NumFreeIndices = 0;
    int32* Hash_ = nullptr;
    int32 HashSize = 0;
};

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
