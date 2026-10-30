#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stop_token>
#include <string_view>
#include <vector>

#include "ue5parse/compress/Decompressor.h"
#include "ue5parse/core/ByteReader.h"
#include "ue5parse/core/FGuid.h"
#include "ue5parse/core/Result.h"
#include "ue5parse/crypto/Decryptor.h"
#include "ue5parse/io/FileSource.h"

namespace ue5parse {

// FPakInfo::EPakFileVersion. UE 5.0-5.6 write Fnv64BugFix, 5.7 and 5.8 Utf8PakDirectory. The reader takes
// PathHashIndex and later; the older ones belong to the Legacy module (design §12).
enum class EPakFileVersion : std::int32_t {
    Initial = 1,
    NoTimestamps = 2,
    CompressionEncryption = 3,
    IndexEncryption = 4,
    RelativeChunkOffsets = 5,
    DeleteRecords = 6,
    EncryptionKeyGuid = 7,
    FNameBasedCompressionMethod = 8,
    FrozenIndex = 9,
    PathHashIndex = 10,
    Fnv64BugFix = 11,
    Utf8PakDirectory = 12,
};

// FPakInfo: the footer at the end of a .pak.
struct FPakInfo {
    static constexpr std::uint32_t magic = 0x5A6F12E1;
    static constexpr std::size_t methodNameLength = 32;
    static constexpr std::size_t maxMethods = 5;
    // EncryptionKeyGuid, bEncryptedIndex, Magic, Version, IndexOffset, IndexSize, IndexHash, the method names.
    static constexpr std::size_t serializedSize = 16 + 1 + 4 + 4 + 8 + 8 + 20 + methodNameLength * maxMethods;

    EPakFileVersion version = EPakFileVersion::PathHashIndex;
    FGuid encryptionKeyGuid;
    bool encryptedIndex = false;
    std::uint64_t indexOffset = 0;
    std::uint64_t indexSize = 0;
    // Index 0 is "None", as in the entries. Zero padded.
    char methodNames[maxMethods + 1][methodNameLength] = {"None"};
    std::uint32_t methodCount = 1;

    std::string_view getMethodName(std::size_t index) const;
};

// The bytes from the start of the footer to the end of the file.
Result<FPakInfo> parsePakInfo(std::span<const std::byte> footer);
Result<FPakInfo> readPakInfo(const FileSource& file);

// One compression block, absolute in the .pak: [start, end). The stored size is end - start, aligned to 16 when the
// entry is encrypted.
struct FPakCompressedBlock {
    std::uint64_t start;
    std::uint64_t end;
};

// FPakEntry, without the hash. The blocks are a range of PakEntries::blocks.
struct FPakEntry {
    std::uint64_t offset = 0;   // of the copy of the entry header that comes before the data
    std::uint64_t size = 0;     // stored, compressed
    std::uint64_t uncompressedSize = 0;
    std::uint32_t compressionBlockSize = 0;
    std::uint32_t firstBlock = 0;
    std::uint32_t blockCount = 0;
    std::uint8_t compressionMethodIndex = 0;
    bool encrypted = false;

    // The serialized size of the header before the data (FPakEntry::GetSerializedSize).
    std::uint64_t getHeaderSize() const;
};

struct PakEntries {
    std::vector<FPakEntry> entries;
    std::vector<FPakCompressedBlock> blocks;
};

// FPakFile::DecodePakEntry: the entry at offset in the encoded entries of the primary index. Appends it and returns
// its index.
Result<std::uint32_t> decodePakEntry(std::span<const std::byte> encoded, std::size_t offset, const FPakInfo& info,
                                     PakEntries& out);
// FPakEntry::Serialize: a full entry, as in the non-encoded entries of the primary index. Appends it.
Result<std::uint32_t> readPakEntry(ByteReader& reader, const FPakInfo& info, PakEntries& out);

// The primary index of a .pak (FPakFile::LoadIndexInternal), decrypted. Views into the index bytes.
struct PakPrimaryIndex {
    std::span<const std::byte> mountPoint;   // FString bytes, as read
    bool mountPointWide = false;
    std::uint32_t entryCount = 0;
    std::uint64_t directoryIndexOffset = 0;
    std::uint64_t directoryIndexSize = 0;
    std::span<const std::byte> encodedEntries;
    // The non-encoded entries: FPakEntryLocation -(i + 1) is entry i of this table.
    PakEntries nonEncoded;
};

// A pak without a full directory index cannot list its files, so it is UnsupportedVersion: UE writes both indexes
// unless told otherwise.
Result<PakPrimaryIndex> parsePakPrimaryIndex(std::span<const std::byte> index, const FPakInfo& info);

// Owns one .pak file. Built from its parsed footer and entries, so it cannot fail.
class PakFile {
public:
    PakFile(FPakInfo info, PakEntries entries, std::unique_ptr<FileSource> file,
            std::vector<const Decompressor*> decompressors, const Decryptor* decryptor);
    PakFile(PakFile&&) noexcept = default;
    ~PakFile();

    const FPakInfo& getInfo() const { return info; }
    std::size_t getEntryCount() const { return entries.entries.size(); }
    const FPakEntry& getEntry(std::size_t index) const { return entries.entries[index]; }
    std::span<const FPakCompressedBlock> getBlocks(const FPakEntry& entry) const {
        return std::span(entries.blocks).subspan(entry.firstBlock, entry.blockCount);
    }
    std::uint64_t getFileSize(std::size_t index) const { return entries.entries[index].uncompressedSize; }

    // Decompresses [offset, offset + output.size()) of the entry into output. Reads only the blocks of that range.
    Result<void> read(std::size_t entryIndex, std::uint64_t offset, std::span<std::byte> output,
                      std::stop_token stop = {}) const;

private:
    class PakEntryBlocks;
    Result<void> decrypt(std::uint64_t position, std::span<std::byte> raw) const;

    FPakInfo info;
    PakEntries entries;
    std::unique_ptr<FileSource> file;
    std::vector<const Decompressor*> decompressors;   // by method index; null for index 0 (none)
    const Decryptor* decryptor;
};

}
