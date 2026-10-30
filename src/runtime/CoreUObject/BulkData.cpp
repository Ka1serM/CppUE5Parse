#include <algorithm>

#include "ue5parse/package/Package.h"
#include "ue5parse/provider/FileProvider.h"
#include "ue5parse/runtime/CoreUObject/Serialization/BulkData.h"
#include "ue5parse/runtime/CoreUObject/UObject/LinkerLoad.h"

namespace ue5parse::runtime {

namespace {

// The chunk that holds a payload outside the export data.
EIoChunkType GetPayloadChunkType(uint32 Flags) {
    if ((Flags & BULKDATA_OptionalPayload) != 0) return EIoChunkType::OptionalBulkData;
    if ((Flags & BULKDATA_MemoryMappedPayload) != 0) return EIoChunkType::MemoryMappedBulkData;
    return EIoChunkType::BulkData;
}

}

void FByteBulkData::Serialize(FArchive& Ar) {
    Linker = Ar.GetLinker();
    if (Linker == nullptr) {
        Ar.SetError(ErrorCode::InvalidData, "FByteBulkData.Linker");
        return;
    }
    // UE 5.2 and later: an index into the package's bulk data map, when the package has one.
    std::span<const FBulkDataMapEntry> Map = Linker->GetPackage().getHeader().getBulkDataMap();
    bool bFromMap = false;
    if (!Map.empty()) {
        int64 At = Ar.Tell();
        int32 Index = 0;
        Ar << Index;
        if (Index >= 0 && static_cast<std::size_t>(Index) < Map.size()) {
            const FBulkDataMapEntry& Entry = Map[static_cast<std::size_t>(Index)];
            BulkDataFlags = Entry.flags;
            SizeOnDisk = static_cast<int64>(Entry.serialSize);
            OffsetInFile = static_cast<int64>(Entry.serialOffset);
            CookedIndex = static_cast<uint8>(Entry.pad & 0xFF);
            bFromMap = true;
        } else {
            Ar.Seek(At);
        }
    }
    if (!bFromMap) {
        Ar << BulkDataFlags;
        const bool bSize64 = (BulkDataFlags & BULKDATA_Size64Bit) != 0;
        if (bSize64) {
            int64 ElementCount = 0;
            Ar << ElementCount << SizeOnDisk;
        } else {
            int32 ElementCount = 0;
            uint32 Size = 0;
            Ar << ElementCount << Size;
            SizeOnDisk = Size;
        }
        Ar << OffsetInFile;
        if ((BulkDataFlags & BULKDATA_BadDataVersion) != 0) {
            Ar.SkipBytes(2);
            BulkDataFlags &= ~static_cast<uint32>(BULKDATA_BadDataVersion);
        }
        if ((BulkDataFlags & BULKDATA_DuplicateNonOptionalPayload) != 0) Ar.SkipBytes(4 + (bSize64 ? 8 : 4) + 8);
    }
    if (SizeOnDisk == 0 || (BulkDataFlags & BULKDATA_Unused) != 0) return;
    if ((BulkDataFlags & BULKDATA_ForceInlinePayload) != 0 || BulkDataFlags == BULKDATA_LazyLoadable || BulkDataFlags == 0) {
        if (SizeOnDisk < 0 || SizeOnDisk > Ar.GetRemaining()) {
            Ar.SetError(ErrorCode::InvalidData, "FByteBulkData.Inline");
            return;
        }
        // The export's bytes are freed once it is serialized, so the payload is a copy.
        std::span<const std::byte> Payload = Ar.ReadView(SizeOnDisk);
        InlinePayload.assign(Payload.begin(), Payload.end());
    }
}

bool FByteBulkData::IsAvailable() const {
    if (Linker == nullptr || SizeOnDisk <= 0 || (BulkDataFlags & BULKDATA_Unused) != 0) return false;
    if (!InlinePayload.empty()) return true;
    if ((BulkDataFlags & (BULKDATA_PayloadInSeperateFile | BULKDATA_OptionalPayload)) == 0) return false;
    const ue5parse::Package& Package = Linker->GetPackage();
    FIoChunkId Id = FIoChunkId::create(Package.getId().value, CookedIndex, GetPayloadChunkType(BulkDataFlags));
    return Package.getProvider().getFiles().findChunk(Id).has_value();
}

std::size_t FByteBulkData::GetPayloadSize() const {
    if (Linker == nullptr || SizeOnDisk <= 0 || (BulkDataFlags & BULKDATA_Unused) != 0) return 0;
    return static_cast<std::size_t>(SizeOnDisk);
}

Result<std::vector<std::byte>> FByteBulkData::Read() const {
    std::vector<std::byte> Out(GetPayloadSize());
    Result<void> Done = ReadInto(Out);
    if (!Done) return std::unexpected(Done.error());
    return Out;
}

Result<void> FByteBulkData::ReadInto(std::span<std::byte> Output) const {
    if (Output.size() != GetPayloadSize()) return fail(ErrorCode::InvalidData, "FByteBulkData.ReadInto.Size");
    if (Output.empty()) return {};
    if ((BulkDataFlags & (BULKDATA_SerializeCompressedZLIB | BULKDATA_CompressedLZO)) != 0)
        return fail(ErrorCode::UnknownCompression, "FByteBulkData.Compressed");
    if (!InlinePayload.empty()) {
        std::copy(InlinePayload.begin(), InlinePayload.end(), Output.begin());
        return {};
    }
    if ((BulkDataFlags & (BULKDATA_PayloadInSeperateFile | BULKDATA_OptionalPayload)) == 0)
        return fail(ErrorCode::UnsupportedVersion, "FByteBulkData.PayloadAtEndOfFile");
    const ue5parse::Package& Package = Linker->GetPackage();
    const FileSystem& Files = Package.getProvider().getFiles();
    FIoChunkId Id = FIoChunkId::create(Package.getId().value, CookedIndex, GetPayloadChunkType(BulkDataFlags));
    std::optional<ChunkLocation> Location = Files.findChunk(Id);
    if (!Location) return fail(ErrorCode::MissingChunk, "FByteBulkData.Read");
    uint64 ChunkSize = Files.getChunkSize(*Location);
    // A chunk of exactly the payload's size holds only it.
    uint64 Offset = ChunkSize == static_cast<uint64>(SizeOnDisk) ? 0 : static_cast<uint64>(OffsetInFile);
    if (Offset > ChunkSize || static_cast<uint64>(SizeOnDisk) > ChunkSize - Offset) return fail(ErrorCode::InvalidData, "FByteBulkData.Range");
    return Files.readChunk(*Location, Offset, Output);
}

}
