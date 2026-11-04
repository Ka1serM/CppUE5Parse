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
#include "CppUE5Parse/Core/FGuid.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Crypto/Decryptor.h"
#include "CppUE5Parse/IO/FileSource.h"

namespace ue5parse {

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

struct FIoChunkId {
    std::uint8_t bytes[12] = {};

    static FIoChunkId create(std::uint64_t id, std::uint16_t index, EIoChunkType type);
    std::uint64_t getId() const;
    std::uint16_t getIndex() const { return static_cast<std::uint16_t>((bytes[8] << 8) | bytes[9]); }
    EIoChunkType getType() const { return static_cast<EIoChunkType>(bytes[11]); }
    std::uint64_t hashWithSeed(std::int32_t seed) const;
    bool operator==(const FIoChunkId&) const = default;
};
static_assert(sizeof(FIoChunkId) == 12);

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

struct FIoDirectoryIndexEntry {
    std::uint32_t name;
    std::uint32_t firstChild;
    std::uint32_t nextSibling;
    std::uint32_t firstFile;
};
struct FIoFileIndexEntry {
    std::uint32_t name;
    std::uint32_t nextFile;
    std::uint32_t userData;
};

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
    std::int64_t findChunk(const FIoChunkId& id) const;

    bool hasDirectoryIndex() const { return !directoryIndex.empty(); }

private:
    friend Result<IoStoreToc> readToc(const FileSource& file);
    friend class IoStoreContainer;

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
    std::span<const std::byte> offsetLengths;
    std::span<const std::byte> perfectHashSeeds;
    std::span<const std::byte> chunksWithoutPerfectHash;
    std::span<const std::byte> blocks;
    std::size_t blockCount = 0;
    std::span<const std::byte> ivs;
    std::vector<std::string_view> methodNames;
    std::vector<std::byte> directoryIndex;
};

Result<IoStoreToc> readToc(const FileSource& file);

class IoStoreContainer {
public:
    IoStoreContainer(IoStoreToc toc, std::vector<std::unique_ptr<FileSource>> partitions,
                     std::vector<const Decompressor*> decompressors, const Decryptor* decryptor);
    IoStoreContainer(IoStoreContainer&&) noexcept = default;
    ~IoStoreContainer();

    const IoStoreToc& getToc() const { return toc; }
    std::size_t getChunkCount() const { return toc.getChunkCount(); }
    std::uint64_t getChunkSize(std::size_t index) const { return toc.getOffsetAndLength(index).length; }

    Result<void> read(std::size_t chunkIndex, std::uint64_t offset, std::span<std::byte> output,
                      std::stop_token stop = {}) const;
    Result<std::vector<std::byte>> takeDirectoryIndex();

private:
    IoStoreToc toc;
    std::vector<std::unique_ptr<FileSource>> partitions;
    std::vector<const Decompressor*> decompressors;
    const Decryptor* decryptor;
};

}
