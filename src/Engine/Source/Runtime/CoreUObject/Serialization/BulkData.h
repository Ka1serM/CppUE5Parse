#pragma once

#include <span>
#include <vector>

#include "CppUE5Parse/Core/Result.h"
#include "Engine/Source/Runtime/Core/Serialization/Archive.h"

namespace ue5parse::runtime {

class FLinkerLoad;

enum EBulkDataFlags : uint32 {
    BULKDATA_None = 0,
    BULKDATA_PayloadAtEndOfFile = 1u << 0,
    BULKDATA_SerializeCompressedZLIB = 1u << 1,
    BULKDATA_ForceSingleElementSerialization = 1u << 2,
    BULKDATA_SingleUse = 1u << 3,
    BULKDATA_CompressedLZO = 1u << 4,
    BULKDATA_Unused = 1u << 5,
    BULKDATA_ForceInlinePayload = 1u << 6,
    BULKDATA_ForceStreamPayload = 1u << 7,
    BULKDATA_PayloadInSeperateFile = 1u << 8,
    BULKDATA_SerializeCompressedBitWindow = 1u << 9,
    BULKDATA_Force_NOT_InlinePayload = 1u << 10,
    BULKDATA_OptionalPayload = 1u << 11,
    BULKDATA_MemoryMappedPayload = 1u << 12,
    BULKDATA_Size64Bit = 1u << 13,
    BULKDATA_DuplicateNonOptionalPayload = 1u << 14,
    BULKDATA_BadDataVersion = 1u << 15,
    BULKDATA_NoOffsetFixUp = 1u << 16,
    BULKDATA_WorkspaceDomainPayload = 1u << 17,
    BULKDATA_LazyLoadable = 1u << 18,
};

class FByteBulkData {
public:
    void Serialize(FArchive& Ar);

    int64 GetElementCount() const { return SizeOnDisk; }
    bool IsAvailable() const;
    std::size_t GetPayloadSize() const;
    Result<std::vector<std::byte>> Read() const;
    Result<void> ReadInto(std::span<std::byte> Output) const;

private:
    FLinkerLoad* Linker = nullptr;
    uint32 BulkDataFlags = 0;
    int64 SizeOnDisk = 0;
    int64 OffsetInFile = 0;
    uint8 CookedIndex = 0;
    int64 InlineChunkOffset = -1;
};

}
