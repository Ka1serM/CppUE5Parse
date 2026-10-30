#include <algorithm>
#include <bit>
#include <cstdlib>

#include "ue5parse/runtime/Core/Containers/Array.h"
#include "ue5parse/runtime/Core/Containers/ScriptSet.h"

namespace ue5parse::runtime {

void* RuntimeMalloc(std::size_t size, std::size_t alignment) {
    alignment = std::max<std::size_t>(alignment, 16);
    size = (std::max<std::size_t>(size, 1) + alignment - 1) / alignment * alignment;
    void* memory = std::aligned_alloc(alignment, size);
    UE5PARSE_CHECK(memory != nullptr);
    return memory;
}

void* RuntimeRealloc(void* memory, std::size_t oldSize, std::size_t newSize, std::size_t alignment) {
    void* grown = RuntimeMalloc(newSize, alignment);
    if (memory != nullptr) {
        std::memcpy(grown, memory, std::min(oldSize, newSize));
        RuntimeFree(memory);
    }
    return grown;
}

void RuntimeFree(void* memory) { std::free(memory); }

namespace {

// TArray's default slack growth: about 3/8 more, and at least 4 elements.
int32 CalculateSlackGrow(int32 NumElements) {
    if (NumElements <= 4) return 4;
    int64 Grown = int64(NumElements) + 3 * int64(NumElements) / 8 + 16;
    return static_cast<int32>(std::min<int64>(Grown, 0x7fffffff));
}

}

void FScriptArray::ResizeTo(int32 NewMax, int32 ElementSize, int32 Alignment) {
    if (NewMax == ArrayMax) return;
    if (NewMax == 0) {
        RuntimeFree(Data);
        Data = nullptr;
    } else {
        Data = RuntimeRealloc(Data, std::size_t(ArrayNum) * ElementSize, std::size_t(NewMax) * ElementSize, Alignment);
    }
    ArrayMax = NewMax;
}

int32 FScriptArray::Add(int32 Count, int32 ElementSize, int32 Alignment) {
    UE5PARSE_CHECK(Count >= 0);
    int32 OldNum = ArrayNum;
    if (OldNum + Count > ArrayMax) ResizeTo(CalculateSlackGrow(OldNum + Count), ElementSize, Alignment);
    ArrayNum += Count;
    return OldNum;
}

int32 FScriptArray::AddZeroed(int32 Count, int32 ElementSize, int32 Alignment) {
    int32 Index = Add(Count, ElementSize, Alignment);
    if (Count > 0) std::memset(static_cast<uint8*>(Data) + std::size_t(Index) * ElementSize, 0, std::size_t(Count) * ElementSize);
    return Index;
}

void FScriptArray::Insert(int32 Index, int32 Count, int32 ElementSize, int32 Alignment) {
    UE5PARSE_CHECK(Index >= 0 && Index <= ArrayNum && Count >= 0);
    int32 OldNum = ArrayNum;
    Add(Count, ElementSize, Alignment);
    uint8* Bytes = static_cast<uint8*>(Data);
    std::memmove(Bytes + std::size_t(Index + Count) * ElementSize, Bytes + std::size_t(Index) * ElementSize,
                 std::size_t(OldNum - Index) * ElementSize);
}

void FScriptArray::Remove(int32 Index, int32 Count, int32 ElementSize, int32 Alignment) {
    if (Count <= 0) return;
    UE5PARSE_CHECK(Index >= 0 && Index + Count <= ArrayNum);
    uint8* Bytes = static_cast<uint8*>(Data);
    std::memmove(Bytes + std::size_t(Index) * ElementSize, Bytes + std::size_t(Index + Count) * ElementSize,
                 std::size_t(ArrayNum - Index - Count) * ElementSize);
    ArrayNum -= Count;
    (void)Alignment;
}

void FScriptArray::Empty(int32 Slack, int32 ElementSize, int32 Alignment) {
    ArrayNum = 0;
    if (Slack != ArrayMax) ResizeTo(Slack, ElementSize, Alignment);
}

void FScriptArray::Reserve(int32 Capacity, int32 ElementSize, int32 Alignment) {
    if (Capacity > ArrayMax) ResizeTo(Capacity, ElementSize, Alignment);
}

void FScriptArray::SetNumUninitialized(int32 NewNum, int32 ElementSize, int32 Alignment) {
    UE5PARSE_CHECK(NewNum >= 0);
    if (NewNum > ArrayMax) ResizeTo(NewNum, ElementSize, Alignment);
    ArrayNum = NewNum;
}

void FScriptArray::SwapMemory(int32 A, int32 B, int32 ElementSize) {
    if (A == B) return;
    uint8* Bytes = static_cast<uint8*>(Data);
    std::swap_ranges(Bytes + std::size_t(A) * ElementSize, Bytes + std::size_t(A + 1) * ElementSize,
                     Bytes + std::size_t(B) * ElementSize);
}

void FScriptArray::MoveAssign(FScriptArray& Other) {
    UE5PARSE_CHECK(Data == nullptr);
    Data = Other.Data;
    ArrayNum = Other.ArrayNum;
    ArrayMax = Other.ArrayMax;
    Other.Data = nullptr;
    Other.ArrayNum = 0;
    Other.ArrayMax = 0;
}

namespace {

int32 AlignUp(int32 Value, int32 Alignment) { return (Value + Alignment - 1) / Alignment * Alignment; }

// A free element holds the links of the free list in its first 8 bytes.
struct FFreeListLink {
    int32 PrevFreeIndex;
    int32 NextFreeIndex;
};

}

FScriptSetLayout GetScriptSetLayout(int32 ElementSize, int32 ElementAlignment) {
    FScriptSetLayout Layout;
    Layout.Alignment = std::max<int32>(ElementAlignment, alignof(int32));
    Layout.HashNextIdOffset = AlignUp(ElementSize, alignof(int32));
    Layout.HashIndexOffset = Layout.HashNextIdOffset + 4;
    Layout.Size = AlignUp(std::max<int32>(Layout.HashIndexOffset + 4, sizeof(FFreeListLink)), Layout.Alignment);
    return Layout;
}

FScriptMapLayout GetScriptMapLayout(int32 KeySize, int32 KeyAlignment, int32 ValueSize, int32 ValueAlignment) {
    FScriptMapLayout Layout;
    Layout.ValueOffset = AlignUp(KeySize, ValueAlignment);
    int32 PairAlignment = std::max(KeyAlignment, ValueAlignment);
    Layout.SetLayout = GetScriptSetLayout(AlignUp(Layout.ValueOffset + ValueSize, PairAlignment), PairAlignment);
    return Layout;
}

bool FScriptSet::IsAllocated(int32 Index) const {
    const uint32* Words = static_cast<const uint32*>(AllocationFlags.GetData());
    return (Words[Index / 32] >> (Index % 32)) & 1;
}

void FScriptSet::SetAllocated(int32 Index, bool Allocated) {
    int32 WordsNeeded = Index / 32 + 1;
    if (AllocationFlags.Num() < WordsNeeded)
        AllocationFlags.AddZeroed(WordsNeeded - AllocationFlags.Num(), sizeof(uint32), alignof(uint32));
    uint32* Words = static_cast<uint32*>(AllocationFlags.GetData());
    if (Allocated) {
        Words[Index / 32] |= 1u << (Index % 32);
    } else {
        Words[Index / 32] &= ~(1u << (Index % 32));
    }
}

bool FScriptSet::IsValidIndex(int32 Index) const { return Index >= 0 && Index < Elements.Num() && IsAllocated(Index); }

void FScriptSet::Empty(int32 Slack, const FScriptSetLayout& Layout) {
    Elements.Empty(Slack, Layout.Size, Layout.Alignment);
    AllocationFlags.Empty(0, sizeof(uint32), alignof(uint32));
    FirstFreeIndex = INDEX_NONE;
    NumFreeIndices = 0;
    ResetHash(Slack);
}

int32 FScriptSet::AddUninitialized(const FScriptSetLayout& Layout) {
    int32 Index;
    if (NumFreeIndices > 0) {
        Index = FirstFreeIndex;
        auto* Link = static_cast<FFreeListLink*>(GetData(Index, Layout));
        FirstFreeIndex = Link->NextFreeIndex;
        NumFreeIndices--;
        if (NumFreeIndices > 0) static_cast<FFreeListLink*>(GetData(FirstFreeIndex, Layout))->PrevFreeIndex = INDEX_NONE;
    } else {
        Index = Elements.Add(1, Layout.Size, Layout.Alignment);
    }
    SetAllocated(Index, true);
    return Index;
}

void FScriptSet::RemoveAt(int32 Index, const FScriptSetLayout& Layout) {
    UE5PARSE_CHECK(IsValidIndex(Index));
    uint8* Element = static_cast<uint8*>(GetData(Index, Layout));
    if (HashSize > 0) {
        // Unlink from the bucket chain.
        uint32 Bucket = *reinterpret_cast<int32*>(Element + Layout.HashIndexOffset);
        int32* Link = &Hash_[Bucket & static_cast<uint32>(HashSize - 1)];
        while (*Link != INDEX_NONE) {
            if (*Link == Index) {
                *Link = *reinterpret_cast<int32*>(Element + Layout.HashNextIdOffset);
                break;
            }
            Link = reinterpret_cast<int32*>(static_cast<uint8*>(GetData(*Link, Layout)) + Layout.HashNextIdOffset);
        }
    }
    auto* Free = reinterpret_cast<FFreeListLink*>(Element);
    Free->PrevFreeIndex = INDEX_NONE;
    Free->NextFreeIndex = NumFreeIndices > 0 ? FirstFreeIndex : INDEX_NONE;
    if (NumFreeIndices > 0) static_cast<FFreeListLink*>(GetData(FirstFreeIndex, Layout))->PrevFreeIndex = Index;
    FirstFreeIndex = Index;
    NumFreeIndices++;
    SetAllocated(Index, false);
}

int32 FScriptSet::GetNumberOfHashBuckets(int32 NumElements) {
    // TSetAllocator: one bucket below 4 elements, else about two elements per bucket plus 8, a power of two.
    if (NumElements < 4) return 1;
    return static_cast<int32>(std::bit_ceil(static_cast<uint32>(NumElements / 2 + 8)));
}

void FScriptSet::ResetHash(int32 NumElements) {
    int32 NewSize = NumElements > 0 ? GetNumberOfHashBuckets(NumElements) : 0;
    if (NewSize != HashSize) {
        RuntimeFree(Hash_);
        Hash_ = NewSize > 0 ? static_cast<int32*>(RuntimeMalloc(sizeof(int32) * NewSize)) : nullptr;
        HashSize = NewSize;
    }
    for (int32 i = 0; i < HashSize; i++) Hash_[i] = INDEX_NONE;
}

void FScriptSet::MoveAssign(FScriptSet& Other, const FScriptSetLayout& Layout) {
    Empty(0, Layout);
    Elements.MoveAssign(Other.Elements);
    AllocationFlags.Empty(0, sizeof(uint32), alignof(uint32));
    AllocationFlags.MoveAssign(Other.AllocationFlags);
    FirstFreeIndex = Other.FirstFreeIndex;
    NumFreeIndices = Other.NumFreeIndices;
    RuntimeFree(Hash_);
    Hash_ = Other.Hash_;
    HashSize = Other.HashSize;
    Other.FirstFreeIndex = INDEX_NONE;
    Other.NumFreeIndices = 0;
    Other.Hash_ = nullptr;
    Other.HashSize = 0;
}

FScriptSet::~FScriptSet() {
    RuntimeFree(Elements.GetData());
    RuntimeFree(AllocationFlags.GetData());
    RuntimeFree(Hash_);
}

}
