#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stop_token>
#include <string_view>
#include <vector>

#include "ue5parse/compress/Decompressor.h"
#include "ue5parse/core/FGuid.h"
#include "ue5parse/core/Result.h"
#include "ue5parse/crypto/Decryptor.h"
#include "ue5parse/io/FileSource.h"

namespace ue5parse {

// EIoChunkType5 (UE5).
enum class EIoChunkType : std::uint8_t {
    Invalid = 0,
    ExportBundleData = 1,
    BulkData = 2,
    OptionalBulkData = 3,
    MemoryMappedBulkData = 4,
    ScriptObjects = 5,
    ContainerHeader = 6,
    ExternalFile = 7,
    ShaderCodeLibrary = 8,
    ShaderCode = 9,
    PackageStoreEntry = 10,
    DerivedData = 11,
    EditorDerivedData = 12,
    PackageResource = 13,
};

// FIoChunkId: 12 bytes, as stored in the TOC. The id, then the index in network byte order, then the type.
struct FIoChunkId {
    std::uint8_t bytes[12] = {};

    static FIoChunkId create(std::uint64_t id, std::uint16_t index, EIoChunkType type);
    std::uint64_t getId() const;
    std::uint16_t getIndex() const { return static_cast<std::uint16_t>((bytes[8] << 8) | bytes[9]); }
    EIoChunkType getType() const { return static_cast<EIoChunkType>(bytes[11]); }
    // The FNV-1a variant of FIoChunkId::HashWithSeed, for the TOC perfect hash.
    std::uint64_t hashWithSeed(std::int32_t seed) const;
    bool operator==(const FIoChunkId&) const = default;
};
static_assert(sizeof(FIoChunkId) == 12);

// EIoStoreTocVersion.
enum class EIoStoreTocVersion : std::uint8_t {
    Invalid = 0,
    Initial,
    DirectoryIndex,
    PartitionSize,
    PerfectHash,
    PerfectHashWithOverflow,
    OnDemandMetaData,
    RemovedOnDemandMetaData,
    ReplaceIoChunkHashWithIoHash,
    AddedSourceHashes,
    ContainerEncryptionMethod,
};

// EIoContainerFlags.
enum class EIoContainerFlags : std::uint8_t { None = 0, Compressed = 1, Encrypted = 2, Signed = 4, Indexed = 8, OnDemand = 16 };

struct FIoStoreTocCompressedBlockEntry {
    std::uint64_t offset;
    std::uint32_t compressedSize;
    std::uint32_t uncompressedSize;
    std::uint8_t compressionMethodIndex;
};

struct FIoOffsetAndLength {
    std::uint64_t offset;
    std::uint64_t length;
};

// One directory of FIoDirectoryIndexEntry, and one file of FIoFileIndexEntry. ~0u is "none".
struct FIoDirectoryIndexEntry {
    std::uint32_t name;
    std::uint32_t firstChild;
    std::uint32_t nextSibling;
    std::uint32_t firstFile;
};
struct FIoFileIndexEntry {
    std::uint32_t name;
    std::uint32_t nextFile;
    std::uint32_t userData;   // the chunk index
};

// The validated contents of one .utoc. Owns the TOC bytes; the arrays are views into them.
class IoStoreToc {
public:
    IoStoreToc() = default;
    IoStoreToc(IoStoreToc&&) noexcept = default;
    IoStoreToc& operator=(IoStoreToc&&) noexcept = default;

    EIoStoreTocVersion getVersion() const { return version; }
    std::uint64_t getContainerId() const { return containerId; }
    FGuid getEncryptionKeyGuid() const { return encryptionKeyGuid; }
    bool isEncrypted() const { return (flags & static_cast<std::uint8_t>(EIoContainerFlags::Encrypted)) != 0; }
    bool isIndexed() const { return (flags & static_cast<std::uint8_t>(EIoContainerFlags::Indexed)) != 0; }
    EncryptionMethod getEncryptionMethod() const { return encryptionMethod; }
    std::uint32_t getCompressionBlockSize() const { return compressionBlockSize; }
    std::uint32_t getPartitionCount() const { return partitionCount; }
    std::uint64_t getPartitionSize() const { return partitionSize; }

    std::size_t getChunkCount() const { return chunkIds.size(); }
    const FIoChunkId& getChunkId(std::size_t index) const { return chunkIds[index]; }
    FIoOffsetAndLength getOffsetAndLength(std::size_t index) const;
    std::size_t getBlockCount() const { return blockCount; }
    FIoStoreTocCompressedBlockEntry getBlock(std::size_t index) const;
    std::span<const std::byte> getIv(std::size_t index) const { return ivs.subspan(index * 12, 12); }
    std::size_t getCompressionMethodCount() const { return methodNames.size(); }
    std::string_view getCompressionMethodName(std::size_t index) const { return methodNames[index]; }
    // The chunk index of an id, or -1. Uses the TOC's perfect hash when it has one, else a linear search.
    std::int64_t findChunk(const FIoChunkId& id) const;

    std::span<const std::byte> getDirectoryIndexBytes() const { return directoryIndex; }

private:
    friend Result<IoStoreToc> parseToc(std::unique_ptr<std::byte[]> bytes, std::size_t size, std::string_view name);

    std::unique_ptr<std::byte[]> bytes;
    EIoStoreTocVersion version = EIoStoreTocVersion::Invalid;
    std::uint64_t containerId = 0;
    FGuid encryptionKeyGuid;
    std::uint8_t flags = 0;
    EncryptionMethod encryptionMethod = EncryptionMethod::None;
    std::uint32_t compressionBlockSize = 0;
    std::uint32_t partitionCount = 1;
    std::uint64_t partitionSize = ~0ull;
    std::span<const FIoChunkId> chunkIds;
    std::span<const std::byte> offsetLengths;   // 10 bytes each, big endian 5 + 5
    std::span<const std::byte> perfectHashSeeds;   // int32 each
    std::span<const std::byte> chunksWithoutPerfectHash;   // int32 each
    std::span<const std::byte> blocks;   // 12 bytes each
    std::size_t blockCount = 0;
    std::span<const std::byte> ivs;   // 12 bytes each
    std::vector<std::string_view> methodNames;   // index 0 is "None"
    std::span<const std::byte> directoryIndex;   // still encrypted when the container is
};

// Parses a whole .utoc that is already in memory.
Result<IoStoreToc> parseToc(std::unique_ptr<std::byte[]> bytes, std::size_t size, std::string_view name);
// Reads and parses a .utoc file.
Result<IoStoreToc> readToc(const FileSource& file);

// Owns the .ucas files (partitions) of one container. Built from a parsed TOC, so it cannot fail.
class IoStoreContainer {
public:
    IoStoreContainer(IoStoreToc toc, std::vector<std::unique_ptr<FileSource>> partitions,
                     std::vector<const Decompressor*> decompressors, const Decryptor* decryptor);
    IoStoreContainer(IoStoreContainer&&) noexcept = default;
    ~IoStoreContainer();

    const IoStoreToc& getToc() const { return toc; }
    std::size_t getChunkCount() const { return toc.getChunkCount(); }
    std::uint64_t getChunkSize(std::size_t index) const { return toc.getOffsetAndLength(index).length; }

    // Decompresses [offset, offset + output.size()) of the chunk into output. Reads only the blocks of that range.
    Result<void> read(std::size_t chunkIndex, std::uint64_t offset, std::span<std::byte> output,
                      std::stop_token stop = {}) const;
    // Decrypts a copy of a range of the TOC itself (the directory index).
    Result<std::vector<std::byte>> decryptDirectoryIndex() const;

private:
    IoStoreToc toc;
    std::vector<std::unique_ptr<FileSource>> partitions;
    std::vector<const Decompressor*> decompressors;   // by method index; null for index 0 (none)
    const Decryptor* decryptor;
};

}
