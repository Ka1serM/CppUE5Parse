#pragma once

#include <array>
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
#include "CppUE5Parse/Core/Versions.h"
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
    bool byteMethodIndex = false;
    std::uint64_t indexOffset = 0;
    std::uint64_t indexSize = 0;
    char methodNames[maxMethods + 1][methodNameLength] = {"None"};
    std::uint32_t methodCount = 1;
    EGame game = EGame::GAME_UE4_0;
    std::uint32_t footerSize = 0;
    //! Tower of Fantasy pak version 3 drops None from the method list, so its entries count methods from one.
    std::uint32_t methodIndexBase = 0;
    std::array<std::byte, 28> customEncryptionData{};
    std::uint32_t customEncryptionDataSize = 0;

    std::string_view getMethodName(std::size_t index) const;
};

Result<FPakInfo> parsePakInfo(std::span<const std::byte> footer, EGame game);
Result<FPakInfo> readPakInfo(const FileSource& file, EGame game);

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
    bool encoded = false;
    //! Wuthering Waves stores a byte after the bitfield that selects how much of the entry is encrypted.
    std::uint8_t customData = 0;
    //! Marvel Rivals, Wuthering Waves, Operation Apocalypse and Minds Eye encrypt only the first bytes of the stored data.
    std::uint32_t encryptedLimit = UINT32_MAX;

    std::uint64_t getHeaderSize(const FPakInfo& info) const;
};

struct PakEntries {
    std::vector<FPakEntry> entries;
    std::vector<FPakCompressedBlock> blocks;
};

Result<std::uint32_t> decodePakEntry(std::span<const std::byte> encoded, std::size_t offset, const FPakInfo& info, PakEntries& out);
Result<std::uint32_t> readPakEntry(ByteReader& reader, const FPakInfo& info, PakEntries& out);

bool hasPartialEncryption(EGame game);
//! The path is the mount point, the directory and the file name, as CUE4Parse joins them.
void setEncryptedLimit(const FPakInfo& info, std::string_view path, FPakEntry& entry);

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
    Result<void> decrypt(std::uint64_t position, std::span<std::byte> raw, std::uint64_t streamStart) const;

    FPakInfo info;
    PakEntries entries;
    std::unique_ptr<FileSource> file;
    std::vector<const Decompressor*> decompressors;
    const Decryptor* decryptor;
};

}
