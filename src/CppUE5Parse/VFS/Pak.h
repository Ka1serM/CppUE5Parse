#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stop_token>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Compress/Decompressor.h"
#include "CppUE5Parse/Core/ByteReader.h"
#include "CppUE5Parse/Core/FGuid.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Crypto/Decryptor.h"
#include "CppUE5Parse/IO/FileSource.h"

namespace ue5parse {

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

struct FPakInfo {
    static constexpr std::uint32_t magic = 0x5A6F12E1;
    static constexpr std::size_t methodNameLength = 32;
    static constexpr std::size_t maxMethods = 5;
    static constexpr std::size_t serializedSize = 16 + 1 + 4 + 4 + 8 + 8 + 20 + methodNameLength * maxMethods;

    EPakFileVersion version = EPakFileVersion::PathHashIndex;
    FGuid encryptionKeyGuid;
    bool encryptedIndex = false;
    std::uint64_t indexOffset = 0;
    std::uint64_t indexSize = 0;
    char methodNames[maxMethods + 1][methodNameLength] = {"None"};
    std::uint32_t methodCount = 1;

    std::string_view getMethodName(std::size_t index) const;
};

Result<FPakInfo> parsePakInfo(std::span<const std::byte> footer);
Result<FPakInfo> readPakInfo(const FileSource& file);

struct FPakCompressedBlock {
    std::uint64_t start;
    std::uint64_t end;
};

struct FPakEntry {
    std::uint64_t offset = 0;
    std::uint64_t size = 0;
    std::uint64_t uncompressedSize = 0;
    std::uint32_t compressionBlockSize = 0;
    std::uint32_t firstBlock = 0;
    std::uint32_t blockCount = 0;
    std::uint8_t compressionMethodIndex = 0;
    bool encrypted = false;

    std::uint64_t getHeaderSize() const;
};

struct PakEntries {
    std::vector<FPakEntry> entries;
    std::vector<FPakCompressedBlock> blocks;
};

Result<std::uint32_t> decodePakEntry(std::span<const std::byte> encoded, std::size_t offset, const FPakInfo& info, PakEntries& out);
Result<std::uint32_t> readPakEntry(ByteReader& reader, const FPakInfo& info, PakEntries& out);

struct PakPrimaryIndex {
    std::span<const std::byte> mountPoint;
    bool mountPointWide = false;
    std::uint32_t entryCount = 0;
    std::uint64_t directoryIndexOffset = 0;
    std::uint64_t directoryIndexSize = 0;
    std::span<const std::byte> encodedEntries;
    PakEntries nonEncoded;
};

Result<PakPrimaryIndex> parsePakPrimaryIndex(std::span<const std::byte> index, const FPakInfo& info);

class PakFile {
public:
    PakFile(FPakInfo info, PakEntries entries, std::unique_ptr<FileSource> file, std::vector<const Decompressor*> decompressors, const Decryptor* decryptor);
    PakFile(PakFile&&) noexcept = default;
    ~PakFile();

    const FPakInfo& getInfo() const { return info; }
    std::size_t getEntryCount() const { return entries.entries.size(); }
    const FPakEntry& getEntry(std::size_t index) const { return entries.entries[index]; }
    std::span<const FPakCompressedBlock> getBlocks(const FPakEntry& entry) const {
        return std::span(entries.blocks).subspan(entry.firstBlock, entry.blockCount);
    }
    std::uint64_t getFileSize(std::size_t index) const { return entries.entries[index].uncompressedSize; }

    Result<void> read(std::size_t entryIndex, std::uint64_t offset, std::span<std::byte> output, std::stop_token stop = {}) const;

private:
    class PakEntryBlocks;
    Result<void> decrypt(std::uint64_t position, std::span<std::byte> raw) const;

    FPakInfo info;
    PakEntries entries;
    std::unique_ptr<FileSource> file;
    std::vector<const Decompressor*> decompressors;
    const Decryptor* decryptor;
};

}
