#include "Engine/Source/Runtime/CoreUObject/Serialization/BulkData.h"

#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/CoreUObjectPrivate.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"

namespace ue5parse::runtime {

namespace {

EIoChunkType GetPayloadChunkType(uint32 Flags) {
    if ((Flags & BULKDATA_OptionalPayload) != 0)
        return EIoChunkType::OptionalBulkData;
    if ((Flags & BULKDATA_MemoryMappedPayload) != 0)
        return EIoChunkType::MemoryMappedBulkData;
    return EIoChunkType::BulkData;
}

}

void FByteBulkData::Serialize(FArchive& Ar) {
    Linker = Ar.GetLinker();
    if (Linker == nullptr) {
        Ar.SetError(ErrorCode::InvalidData, "FByteBulkData.Linker");
        return;
    }
    std::span<const FBulkDataMapEntry> Map = Linker->GetPackage().getBulkDataMap();
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
        if ((BulkDataFlags & BULKDATA_DuplicateNonOptionalPayload) != 0)
            Ar.SkipBytes(4 + (bSize64 ? 8 : 4) + 8);
    }
    if (SizeOnDisk == 0 || (BulkDataFlags & BULKDATA_Unused) != 0)
        return;
    if ((BulkDataFlags & BULKDATA_ForceInlinePayload) != 0 || BulkDataFlags == BULKDATA_LazyLoadable || BulkDataFlags == 0) {
        if (SizeOnDisk < 0 || SizeOnDisk > Ar.GetRemaining()) {
            Ar.SetError(ErrorCode::InvalidData, "FByteBulkData.Inline");
            return;
        }
        InlineChunkOffset = Ar.GetExportChunkPosition();
        if (InlineChunkOffset < 0) {
            Ar.SetError(ErrorCode::InvalidData, "FByteBulkData.InlineOutsideExport");
            return;
        }
        Ar.SkipBytes(SizeOnDisk);
    }
}

bool FByteBulkData::IsAvailable() const {
    if (Linker == nullptr || SizeOnDisk <= 0 || (BulkDataFlags & BULKDATA_Unused) != 0)
        return false;
    if (InlineChunkOffset >= 0)
        return true;
    const ue5parse::Package& Package = Linker->GetPackage();
    if ((BulkDataFlags & (BULKDATA_PayloadInSeperateFile | BULKDATA_OptionalPayload)) == 0)
        return GetEndOfFileOffset() + static_cast<uint64>(SizeOnDisk) <= Package.getExportDataSize();
    return Package.getBulkDataSize(GetPayloadChunkType(BulkDataFlags), CookedIndex).has_value();
}

int64 FByteBulkData::GetOffsetFixUp() const {
    return (BulkDataFlags & BULKDATA_NoOffsetFixUp) != 0 ? 0 : Linker->GetPackage().getBulkDataStartOffset();
}

uint64 FByteBulkData::GetEndOfFileOffset() const {
    const int64 Offset = OffsetInFile + GetOffsetFixUp();
    return Offset < 0 ? UINT64_MAX / 2 : static_cast<uint64>(Offset);
}

std::size_t FByteBulkData::GetPayloadSize() const {
    if (Linker == nullptr || SizeOnDisk <= 0 || (BulkDataFlags & BULKDATA_Unused) != 0)
        return 0;
    return static_cast<std::size_t>(SizeOnDisk);
}

Result<std::vector<std::byte>> FByteBulkData::Read() const {
    std::vector<std::byte> Out(GetPayloadSize());
    Result<void> Done = ReadInto(Out);
    if (!Done)
        return std::unexpected(Done.error());
    return Out;
}

Result<void> FByteBulkData::ReadInto(std::span<std::byte> Output) const {
    if (Output.size() != GetPayloadSize())
        return fail(ErrorCode::InvalidData, "FByteBulkData.ReadInto.Size");
    if (Output.empty())
        return {};
    if ((BulkDataFlags & (BULKDATA_SerializeCompressedZLIB | BULKDATA_CompressedLZO)) != 0)
        return fail(ErrorCode::UnknownCompression, "FByteBulkData.Compressed");
    const ue5parse::Package& Package = Linker->GetPackage();
    if (InlineChunkOffset >= 0) {
        FLoaderUnlock Unlock;
        return Package.readExportData(static_cast<uint64>(InlineChunkOffset), Output);
    }
    if ((BulkDataFlags & (BULKDATA_PayloadInSeperateFile | BULKDATA_OptionalPayload)) == 0) {
        FLoaderUnlock Unlock;
        return Package.readExportData(GetEndOfFileOffset(), Output);
    }
    const EIoChunkType Type = GetPayloadChunkType(BulkDataFlags);
    std::optional<uint64> ChunkSize = Package.getBulkDataSize(Type, CookedIndex);
    if (!ChunkSize)
        return fail(ErrorCode::MissingChunk, "FByteBulkData.Read");
    uint64 Offset = *ChunkSize == static_cast<uint64>(SizeOnDisk) ? 0 : static_cast<uint64>(OffsetInFile + GetOffsetFixUp());
    if (Offset > *ChunkSize || static_cast<uint64>(SizeOnDisk) > *ChunkSize - Offset)
        return fail(ErrorCode::InvalidData, "FByteBulkData.Range");
    FLoaderUnlock Unlock;
    return Package.readBulkData(Type, CookedIndex, Offset, Output);
}

void FFormatContainer::Serialize(FArchive& Ar) {
    int32 NumFormats = 0;
    Ar << NumFormats;
    for (int32 Index = 0; Index < NumFormats && !Ar.IsError(); Index++) {
        FName Name;
        Ar << Name;
        GetFormat(Name).Serialize(Ar);
    }
}

}
