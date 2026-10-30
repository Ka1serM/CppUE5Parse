#pragma once

#include <span>
#include <vector>

#include "ue5parse/core/Result.h"
#include "ue5parse/runtime/Core/Serialization/Archive.h"

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

// FByteBulkData: where a payload is (inline in the export, or in a bulk data chunk of the package). The payload is read
// on request (UE's FBulkData of the Zen loader).
class FByteBulkData {
public:
    void Serialize(FArchive& Ar);

    uint32 GetBulkDataFlags() const { return BulkDataFlags; }
    int64 GetElementCount() const { return SizeOnDisk; }
    int64 GetBulkDataSize() const { return SizeOnDisk; }
    int64 GetBulkDataOffsetInFile() const { return OffsetInFile; }
    bool IsInlined() const { return (BulkDataFlags & BULKDATA_ForceInlinePayload) != 0; }
    // Whether there is a payload to read: not empty, not unused, and its chunk is mounted. Reads nothing.
    bool IsAvailable() const;
    // The payload's size in bytes. 0 when there is none.
    std::size_t GetPayloadSize() const;
    // The payload. MissingChunk when its chunk is not mounted.
    Result<std::vector<std::byte>> Read() const;
    // The payload into a buffer of exactly GetPayloadSize() bytes, without another copy.
    Result<void> ReadInto(std::span<std::byte> Output) const;

private:
    FLinkerLoad* Linker = nullptr;
    uint32 BulkDataFlags = 0;
    int64 SizeOnDisk = 0;
    int64 OffsetInFile = 0;
    uint8 CookedIndex = 0;
    std::span<const std::byte> InlinePayload;
};


}
