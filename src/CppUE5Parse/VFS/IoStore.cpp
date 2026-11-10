#include "CppUE5Parse/VFS/IoStore.h"

#include <algorithm>
#include <cstring>

#include "CppUE5Parse/Core/ByteReader.h"
#include "CppUE5Parse/Core/Check.h"
#include "CppUE5Parse/VFS/BlockReader.h"

namespace ue5parse {

namespace {

constexpr std::size_t tocHeaderSize = 144;
constexpr std::uint8_t tocMagic[16] = {0x2D, 0x3D, 0x3D, 0x2D, 0x2D, 0x3D, 0x3D, 0x2D, 0x2D, 0x3D, 0x3D, 0x2D, 0x2D, 0x3D, 0x3D, 0x2D};

std::uint64_t readBigEndian40(const std::byte* bytes) {
    std::uint64_t value = 0;
    for (int i = 0; i < 5; i++)
        value = (value << 8) | static_cast<std::uint8_t>(bytes[i]);
    return value;
}

std::int32_t readInt32At(std::span<const std::byte> bytes, std::size_t index) {
    std::int32_t value;
    std::memcpy(&value, bytes.data() + index * 4, 4);
    return value;
}

}

FIoChunkId FIoChunkId::create(std::uint64_t id, std::uint16_t index, EIoChunkType type) {
    FIoChunkId chunk;
    std::memcpy(chunk.bytes, &id, 8);
    chunk.bytes[8] = static_cast<std::uint8_t>(index >> 8);
    chunk.bytes[9] = static_cast<std::uint8_t>(index);
    chunk.bytes[11] = static_cast<std::uint8_t>(type);
    return chunk;
}

std::uint64_t FIoChunkId::getId() const {
    std::uint64_t id;
    std::memcpy(&id, bytes, 8);
    return id;
}

std::uint64_t FIoChunkId::hashWithSeed(std::int32_t seed) const {
    std::uint64_t hash = seed != 0 ? static_cast<std::uint64_t>(static_cast<std::int64_t>(seed)) : 0xcbf29ce484222325ull;
    for (std::uint8_t byte : bytes)
        hash = (hash * 0x00000100000001B3ull) ^ byte;
    return hash;
}

FIoOffsetAndLength IoStoreToc::getOffsetAndLength(std::size_t index) const {
    const std::byte* entry = offsetLengths.data() + index * 10;
    return {readBigEndian40(entry), readBigEndian40(entry + 5)};
}

FIoStoreTocCompressedBlockEntry IoStoreToc::getBlock(std::size_t index) const {
    std::uint64_t offsetAndSize;
    std::uint32_t sizeAndMethod;
    std::memcpy(&offsetAndSize, blocks.data() + index * 12, 8);
    std::memcpy(&sizeAndMethod, blocks.data() + index * 12 + 8, 4);
    return {offsetAndSize & ((1ull << 40) - 1), static_cast<std::uint32_t>((offsetAndSize >> 40) & 0xFFFFFF), sizeAndMethod & 0xFFFFFF,
            static_cast<std::uint8_t>(sizeAndMethod >> 24)};
}

std::int64_t IoStoreToc::findChunk(const FIoChunkId& id) const {
    std::size_t count = chunkIds.size();
    if (count == 0)
        return -1;
    std::size_t seedCount = perfectHashSeeds.size() / 4;
    if (seedCount > 0) {
        std::int32_t seed = readInt32At(perfectHashSeeds, id.hashWithSeed(0) % seedCount);
        if (seed == 0)
            return -1;
        std::size_t slot;
        if (seed < 0) {
            std::uint64_t direct = static_cast<std::uint64_t>(-static_cast<std::int64_t>(seed) - 1);
            if (direct >= count) {
                for (std::size_t i = 0; i < chunksWithoutPerfectHash.size() / 4; i++) {
                    std::int32_t index = readInt32At(chunksWithoutPerfectHash, i);
                    if (index >= 0 && static_cast<std::size_t>(index) < count && chunkIds[index] == id)
                        return index;
                }
                return -1;
            }
            slot = static_cast<std::size_t>(direct);
        } else {
            slot = static_cast<std::size_t>(id.hashWithSeed(seed) % count);
        }
        return chunkIds[slot] == id ? static_cast<std::int64_t>(slot) : -1;
    }
    for (std::size_t i = 0; i < count; i++)
        if (chunkIds[i] == id)
            return static_cast<std::int64_t>(i);
    return -1;
}

namespace {

struct TocCounts {
    EIoStoreTocVersion version = EIoStoreTocVersion::Invalid;
    std::uint64_t containerId = 0;
    FGuid encryptionKeyGuid;
    std::uint8_t flags = 0;
    EncryptionMethod encryptionMethod = EncryptionMethod::None;
    std::uint32_t compressionBlockSize = 0;
    std::uint32_t partitionCount = 1;
    std::uint64_t partitionSize = ~0ull;
    std::uint32_t entries = 0;
    std::uint32_t blocks = 0;
    std::uint32_t methodNames = 0;
    std::uint32_t methodNameLength = 0;
    std::uint32_t directoryIndexSize = 0;
    std::uint32_t perfectHashSeeds = 0;
    std::uint32_t withoutPerfectHash = 0;
    std::uint32_t ivs = 0;

    std::uint64_t getTablesEnd() const {
        return tocHeaderSize + std::uint64_t(entries) * (sizeof(FIoChunkId) + 10) + std::uint64_t(perfectHashSeeds) * 4 + std::uint64_t(withoutPerfectHash) * 4
               + std::uint64_t(blocks) * 12 + std::uint64_t(ivs) * 12 + std::uint64_t(methodNames) * methodNameLength;
    }
};

Result<TocCounts> parseTocHeader(std::span<const std::byte> data) {
    ByteReader reader{data};
    if (std::memcmp(data.data(), tocMagic, sizeof(tocMagic)) != 0)
        return fail(ErrorCode::BadMagic, "parseToc.magic");
    reader.skip(16);
    TocCounts counts;
    EIoStoreTocVersion version = static_cast<EIoStoreTocVersion>(reader.read<std::uint8_t>());
    reader.skip(3);
    std::uint32_t headerSize = reader.read<std::uint32_t>();
    counts.entries = reader.read<std::uint32_t>();
    counts.blocks = reader.read<std::uint32_t>();
    std::uint32_t blockEntrySize = reader.read<std::uint32_t>();
    counts.methodNames = reader.read<std::uint32_t>();
    counts.methodNameLength = reader.read<std::uint32_t>();
    std::uint32_t compressionBlockSize = reader.read<std::uint32_t>();
    counts.directoryIndexSize = reader.read<std::uint32_t>();
    std::uint32_t partitionCount = reader.read<std::uint32_t>();
    std::uint64_t containerId = reader.read<std::uint64_t>();
    FGuid encryptionKeyGuid = reader.read<FGuid>();
    std::uint8_t flags = reader.read<std::uint8_t>();
    std::uint8_t storedMethod = reader.read<std::uint8_t>();
    reader.skip(2);
    EncryptionMethod encryptionMethod = EncryptionMethod::None;
    counts.perfectHashSeeds = reader.read<std::uint32_t>();
    std::uint64_t partitionSize = reader.read<std::uint64_t>();
    counts.withoutPerfectHash = reader.read<std::uint32_t>();
    reader.skip(4);
    counts.ivs = reader.read<std::uint32_t>();
    if (reader.hasFailed())
        return std::unexpected(reader.getError());

    if (version == EIoStoreTocVersion::Invalid)
        return fail(ErrorCode::UnsupportedVersion, "parseToc.version");
    if (headerSize != tocHeaderSize)
        return fail(ErrorCode::InvalidData, "parseToc.headerSize");
    if (blockEntrySize != 12)
        return fail(ErrorCode::InvalidData, "parseToc.blockEntrySize");
    if (compressionBlockSize == 0)
        return fail(ErrorCode::InvalidData, "parseToc.blockSize");
    if (version < EIoStoreTocVersion::PartitionSize) {
        partitionCount = 1;
        partitionSize = ~0ull;
    }
    if (partitionCount == 0 || partitionSize == 0)
        return fail(ErrorCode::InvalidData, "parseToc.partitions");
    bool encrypted = (flags & static_cast<std::uint8_t>(EIoContainerFlags::Encrypted)) != 0;
    bool indexed = (flags & static_cast<std::uint8_t>(EIoContainerFlags::Indexed)) != 0;
    if (version < EIoStoreTocVersion::ContainerEncryptionMethod) {
        encryptionMethod = encrypted ? EncryptionMethod::AesEcb : EncryptionMethod::None;
        counts.ivs = 0;
    } else {
        encryptionMethod = static_cast<EncryptionMethod>(storedMethod);
    }
    if (version < EIoStoreTocVersion::PerfectHash)
        counts.perfectHashSeeds = 0;
    if (version < EIoStoreTocVersion::PerfectHashWithOverflow)
        counts.withoutPerfectHash = 0;
    if (version < EIoStoreTocVersion::DirectoryIndex || !indexed)
        counts.directoryIndexSize = 0;
    std::size_t expectedIvs = 0;
    if (encryptionMethod == EncryptionMethod::AesCtr)
        expectedIvs = counts.blocks + (indexed ? 1 : 0);
    if (counts.ivs != expectedIvs)
        return fail(ErrorCode::InvalidData, "parseToc.ivCount");

    counts.version = version;
    counts.compressionBlockSize = compressionBlockSize;
    counts.partitionCount = partitionCount;
    counts.partitionSize = partitionSize;
    counts.containerId = containerId;
    counts.encryptionKeyGuid = encryptionKeyGuid;
    counts.flags = flags;
    counts.encryptionMethod = encryptionMethod;
    return counts;
}

}

Result<IoStoreToc> readToc(const FileSource& file) {
    std::uint64_t fileSize = file.getSize();
    if (fileSize < tocHeaderSize)
        return fail(ErrorCode::ReadPastEnd, "parseToc.header");
    std::byte header[tocHeaderSize];
    Result<void> headerRead = file.read(0, header);
    if (!headerRead)
        return std::unexpected(headerRead.error());
    Result<TocCounts> counts = parseTocHeader(header);
    if (!counts)
        return std::unexpected(counts.error());
    IoStoreToc toc;
    toc.version = counts->version;
    toc.containerId = counts->containerId;
    toc.encryptionKeyGuid = counts->encryptionKeyGuid;
    toc.flags = counts->flags;
    toc.encryptionMethod = counts->encryptionMethod;
    toc.compressionBlockSize = counts->compressionBlockSize;
    toc.partitionCount = counts->partitionCount;
    toc.partitionSize = counts->partitionSize;

    std::uint64_t tablesEnd = counts->getTablesEnd();
    std::uint64_t directoryStart = tablesEnd;
    if ((toc.flags & static_cast<std::uint8_t>(EIoContainerFlags::Signed)) != 0) {
        std::uint32_t hashSize = 0;
        if (tablesEnd + 4 > fileSize)
            return fail(ErrorCode::ReadPastEnd, "parseToc.signatures");
        Result<void> read = file.read(tablesEnd, std::as_writable_bytes(std::span(&hashSize, 1)));
        if (!read)
            return std::unexpected(read.error());
        directoryStart += 4 + std::uint64_t(hashSize) * 2 + std::uint64_t(counts->blocks) * 20;
    }
    if (tablesEnd > fileSize || directoryStart + counts->directoryIndexSize > fileSize)
        return fail(ErrorCode::ReadPastEnd, "parseToc.tables");
    //! A 32-bit target (WebAssembly) cannot hold tables above 4 GiB in memory.
    if (tablesEnd > SIZE_MAX)
        return fail(ErrorCode::InvalidData, "readToc.size");

    std::size_t size = static_cast<std::size_t>(tablesEnd);
    auto bytes = std::make_unique_for_overwrite<std::byte[]>(size);
    Result<void> read = file.read(0, {bytes.get(), size});
    if (!read)
        return std::unexpected(read.error());
    if (counts->directoryIndexSize > 0) {
        toc.directoryIndex.resize(counts->directoryIndexSize);
        Result<void> index = file.read(directoryStart, toc.directoryIndex);
        if (!index)
            return std::unexpected(index.error());
    }

    ByteReader reader{{bytes.get(), size}};
    reader.seek(tocHeaderSize);
    std::span<const std::byte> ids = reader.readBytes(std::size_t(counts->entries) * sizeof(FIoChunkId));
    toc.offsetLengths = reader.readBytes(std::size_t(counts->entries) * 10);
    toc.perfectHashSeeds = reader.readBytes(std::size_t(counts->perfectHashSeeds) * 4);
    toc.chunksWithoutPerfectHash = reader.readBytes(std::size_t(counts->withoutPerfectHash) * 4);
    toc.blocks = reader.readBytes(std::size_t(counts->blocks) * 12);
    toc.blockCount = counts->blocks;
    toc.ivs = reader.readBytes(std::size_t(counts->ivs) * 12);
    std::span<const std::byte> names = reader.readBytes(std::size_t(counts->methodNames) * counts->methodNameLength);
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    static_assert(alignof(FIoChunkId) == 1);
    toc.chunkIds = {reinterpret_cast<const FIoChunkId*>(ids.data()), counts->entries};
    toc.methodNames.push_back("None");
    for (std::uint32_t i = 0; i < counts->methodNames; i++) {
        const char* name = reinterpret_cast<const char*>(names.data() + std::size_t(i) * counts->methodNameLength);
        toc.methodNames.push_back(std::string_view(name, strnlen(name, counts->methodNameLength)));
    }
    toc.bytes = std::move(bytes);
    return toc;
}

IoStoreContainer::IoStoreContainer(IoStoreToc toc, std::vector<std::unique_ptr<FileSource>> partitions, std::vector<const Decompressor*> decompressors,
                                   const Decryptor* decryptor)
    : toc(std::move(toc)), partitions(std::move(partitions)), decompressors(std::move(decompressors)), decryptor(decryptor) {
    UE5PARSE_CHECK(this->partitions.size() == this->toc.getPartitionCount());
    UE5PARSE_CHECK(this->decompressors.size() == this->toc.getCompressionMethodCount());
}

IoStoreContainer::~IoStoreContainer() {
    forgetDecodedBlocks();
}

namespace {

class IoStoreBlocks final : public BlockList {
public:
    IoStoreBlocks(const IoStoreToc& toc, std::span<const std::unique_ptr<FileSource>> partitions, std::span<const Decompressor* const> decompressors,
                  const Decryptor* decryptor)
        : toc(toc), partitions(partitions), decompressors(decompressors), decryptor(decryptor) {}

    Result<StoredBlock> getBlock(std::size_t index) const override {
        if (index >= toc.getBlockCount())
            return fail(ErrorCode::InvalidData, "IoStoreContainer.read.blocks", index);
        FIoStoreTocCompressedBlockEntry entry = toc.getBlock(index);
        if (entry.compressionMethodIndex >= decompressors.size())
            return fail(ErrorCode::UnknownCompression, "IoStoreContainer.read.method", entry.offset);
        std::uint64_t partition = entry.offset / toc.getPartitionSize();
        if (partition >= partitions.size())
            return fail(ErrorCode::InvalidData, "IoStoreContainer.partition", entry.offset);
        if (toc.isEncrypted() && decryptor == nullptr)
            return fail(ErrorCode::MissingKey, "IoStoreContainer.decryptor", entry.offset);
        StoredBlock block;
        block.file = partitions[partition].get();
        block.position = entry.offset % toc.getPartitionSize();
        block.compressedSize = entry.compressedSize;
        block.storedSize = toc.isEncrypted() ? static_cast<std::size_t>(alignUp(entry.compressedSize, aesBlock)) : entry.compressedSize;
        block.uncompressedSize = entry.uncompressedSize;
        block.decompressor = decompressors[entry.compressionMethodIndex];
        block.encrypted = toc.isEncrypted();
        return block;
    }

    Result<void> decrypt(std::size_t index, std::span<std::byte> stored) const override {
        DecryptRequest request;
        request.data = stored;
        request.keyGuid = toc.getEncryptionKeyGuid();
        request.method = toc.getEncryptionMethod();
        if (request.method == EncryptionMethod::AesCtr)
            std::memcpy(request.iv.data(), toc.getIv(index).data(), 12);
        request.part = EncryptedPart::CompressionBlock;
        request.containerId = toc.getContainerId();
        request.offset = toc.getBlock(index).offset;
        return decryptor->decrypt(request);
    }

private:
    const IoStoreToc& toc;
    std::span<const std::unique_ptr<FileSource>> partitions;
    std::span<const Decompressor* const> decompressors;
    const Decryptor* decryptor;
};

}

Result<void> IoStoreContainer::read(std::size_t chunkIndex, std::uint64_t offset, std::span<std::byte> output, std::stop_token stop) const {
    if (chunkIndex >= toc.getChunkCount())
        return fail(ErrorCode::MissingChunk, "IoStoreContainer.read.index");
    FIoOffsetAndLength chunk = toc.getOffsetAndLength(chunkIndex);
    if (offset > chunk.length || output.size() > chunk.length - offset)
        return fail(ErrorCode::ReadPastEnd, "IoStoreContainer.read.range", offset);
    IoStoreBlocks blocks(toc, partitions, decompressors, decryptor);
    return readBlocks(blocks, toc.getCompressionBlockSize(), chunk.offset + offset, output, stop);
}

Result<std::vector<std::byte>> IoStoreContainer::takeDirectoryIndex() {
    std::vector<std::byte> index = std::move(toc.directoryIndex);
    toc.directoryIndex = {};
    if (!toc.isEncrypted())
        return index;
    if (decryptor == nullptr)
        return fail(ErrorCode::MissingKey, "IoStoreContainer.directoryIndex");
    DecryptRequest request;
    request.keyGuid = toc.getEncryptionKeyGuid();
    request.method = toc.getEncryptionMethod();
    request.part = EncryptedPart::TocDirectoryIndex;
    request.containerId = toc.getContainerId();
    if (request.method == EncryptionMethod::AesCtr) {
        std::memcpy(request.iv.data(), toc.getIv(toc.getBlockCount()).data(), 12);
        request.data = index;
    } else {
        if (index.size() % aesBlock != 0)
            return fail(ErrorCode::InvalidData, "IoStoreContainer.directoryIndex.size");
        request.data = index;
    }
    Result<void> done = decryptor->decrypt(request);
    if (!done)
        return std::unexpected(done.error());
    return index;
}

}
