#include <algorithm>
#include <cstring>

#include "ue5parse/core/ByteReader.h"
#include "ue5parse/core/Check.h"
#include "ue5parse/vfs/IoStore.h"

#include "BlockReader.h"

namespace ue5parse {

namespace {

constexpr std::size_t tocHeaderSize = 144;
constexpr std::uint8_t tocMagic[16] = {0x2D, 0x3D, 0x3D, 0x2D, 0x2D, 0x3D, 0x3D, 0x2D,
                                       0x2D, 0x3D, 0x3D, 0x2D, 0x2D, 0x3D, 0x3D, 0x2D};

std::uint64_t readBigEndian40(const std::byte* bytes) {
    std::uint64_t value = 0;
    for (int i = 0; i < 5; i++) value = (value << 8) | static_cast<std::uint8_t>(bytes[i]);
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
    for (std::uint8_t byte : bytes) hash = (hash * 0x00000100000001B3ull) ^ byte;
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
    return {offsetAndSize & ((1ull << 40) - 1), static_cast<std::uint32_t>((offsetAndSize >> 40) & 0xFFFFFF),
            sizeAndMethod & 0xFFFFFF, static_cast<std::uint8_t>(sizeAndMethod >> 24)};
}

std::int64_t IoStoreToc::findChunk(const FIoChunkId& id) const {
    std::size_t count = chunkIds.size();
    if (count == 0) return -1;
    std::size_t seedCount = perfectHashSeeds.size() / 4;
    if (seedCount > 0) {
        std::int32_t seed = readInt32At(perfectHashSeeds, id.hashWithSeed(0) % seedCount);
        if (seed == 0) return -1;
        std::size_t slot;
        if (seed < 0) {
            std::uint64_t direct = static_cast<std::uint64_t>(-static_cast<std::int64_t>(seed) - 1);
            if (direct >= count) {
                // An entry without a perfect hash: a linear search over the overflow list.
                for (std::size_t i = 0; i < chunksWithoutPerfectHash.size() / 4; i++) {
                    std::int32_t index = readInt32At(chunksWithoutPerfectHash, i);
                    if (index >= 0 && static_cast<std::size_t>(index) < count && chunkIds[index] == id) return index;
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
        if (chunkIds[i] == id) return static_cast<std::int64_t>(i);
    return -1;
}

Result<IoStoreToc> parseToc(std::unique_ptr<std::byte[]> bytes, std::size_t size, std::string_view) {
    std::span<const std::byte> data{bytes.get(), size};
    ByteReader reader{data};
    if (size < tocHeaderSize) return fail(ErrorCode::ReadPastEnd, "parseToc.header");
    if (std::memcmp(data.data(), tocMagic, sizeof(tocMagic)) != 0) return fail(ErrorCode::BadMagic, "parseToc.magic");
    reader.skip(16);

    IoStoreToc toc;
    toc.version = static_cast<EIoStoreTocVersion>(reader.read<std::uint8_t>());
    reader.skip(3);
    std::uint32_t headerSize = reader.read<std::uint32_t>();
    std::uint32_t entryCount = reader.read<std::uint32_t>();
    std::uint32_t blockEntryCount = reader.read<std::uint32_t>();
    std::uint32_t blockEntrySize = reader.read<std::uint32_t>();
    std::uint32_t methodNameCount = reader.read<std::uint32_t>();
    std::uint32_t methodNameLength = reader.read<std::uint32_t>();
    toc.compressionBlockSize = reader.read<std::uint32_t>();
    std::uint32_t directoryIndexSize = reader.read<std::uint32_t>();
    toc.partitionCount = reader.read<std::uint32_t>();
    toc.containerId = reader.read<std::uint64_t>();
    toc.encryptionKeyGuid = reader.read<FGuid>();
    toc.flags = reader.read<std::uint8_t>();
    std::uint8_t encryptionMethod = reader.read<std::uint8_t>();
    reader.skip(2);
    std::uint32_t perfectHashSeedCount = reader.read<std::uint32_t>();
    toc.partitionSize = reader.read<std::uint64_t>();
    std::uint32_t withoutPerfectHashCount = reader.read<std::uint32_t>();
    reader.skip(4);   // source hash count (UE 6.0); not read
    std::uint32_t ivCount = reader.read<std::uint32_t>();
    reader.seek(tocHeaderSize);
    if (reader.hasFailed()) return std::unexpected(reader.getError());

    if (toc.version == EIoStoreTocVersion::Invalid) return fail(ErrorCode::UnsupportedVersion, "parseToc.version");
    if (headerSize != tocHeaderSize) return fail(ErrorCode::InvalidData, "parseToc.headerSize");
    if (blockEntrySize != 12) return fail(ErrorCode::InvalidData, "parseToc.blockEntrySize");
    if (toc.compressionBlockSize == 0) return fail(ErrorCode::InvalidData, "parseToc.blockSize");
    if (toc.version < EIoStoreTocVersion::PartitionSize) {
        toc.partitionCount = 1;
        toc.partitionSize = ~0ull;
    }
    if (toc.partitionCount == 0 || toc.partitionSize == 0) return fail(ErrorCode::InvalidData, "parseToc.partitions");
    if (toc.version < EIoStoreTocVersion::ContainerEncryptionMethod) {
        toc.encryptionMethod = toc.isEncrypted() ? EncryptionMethod::AesEcb : EncryptionMethod::None;
        ivCount = 0;
    } else {
        toc.encryptionMethod = static_cast<EncryptionMethod>(encryptionMethod);
    }
    if (toc.version < EIoStoreTocVersion::PerfectHash) perfectHashSeedCount = 0;
    if (toc.version < EIoStoreTocVersion::PerfectHashWithOverflow) withoutPerfectHashCount = 0;

    // Chunk ids and offsets.
    std::span<const std::byte> ids = reader.readBytes(std::size_t(entryCount) * sizeof(FIoChunkId));
    toc.offsetLengths = reader.readBytes(std::size_t(entryCount) * 10);
    toc.perfectHashSeeds = reader.readBytes(std::size_t(perfectHashSeedCount) * 4);
    toc.chunksWithoutPerfectHash = reader.readBytes(std::size_t(withoutPerfectHashCount) * 4);
    toc.blocks = reader.readBytes(std::size_t(blockEntryCount) * 12);
    toc.blockCount = blockEntryCount;
    if (reader.hasFailed()) return std::unexpected(reader.getError());
    static_assert(alignof(FIoChunkId) == 1);
    toc.chunkIds = {reinterpret_cast<const FIoChunkId*>(ids.data()), entryCount};

    std::size_t expectedIvs = 0;
    if (toc.encryptionMethod == EncryptionMethod::AesCtr) expectedIvs = blockEntryCount + (toc.isIndexed() ? 1 : 0);
    if (ivCount != expectedIvs) return fail(ErrorCode::InvalidData, "parseToc.ivCount");
    toc.ivs = reader.readBytes(std::size_t(ivCount) * 12);

    // Compression method names, fixed-size and zero padded. Index 0 is "None".
    toc.methodNames.push_back("None");
    std::span<const std::byte> names = reader.readBytes(std::size_t(methodNameCount) * methodNameLength);
    if (reader.hasFailed()) return std::unexpected(reader.getError());
    for (std::uint32_t i = 0; i < methodNameCount; i++) {
        const char* name = reinterpret_cast<const char*>(names.data() + std::size_t(i) * methodNameLength);
        toc.methodNames.push_back(std::string_view(name, strnlen(name, methodNameLength)));
    }

    if ((toc.flags & static_cast<std::uint8_t>(EIoContainerFlags::Signed)) != 0) {
        std::uint32_t hashSize = reader.read<std::uint32_t>();
        reader.skip(std::size_t(hashSize) * 2);
        reader.skip(std::size_t(blockEntryCount) * 20);
    }
    if (toc.version >= EIoStoreTocVersion::DirectoryIndex && toc.isIndexed() && directoryIndexSize > 0)
        toc.directoryIndex = reader.readBytes(directoryIndexSize);
    if (reader.hasFailed()) return std::unexpected(reader.getError());

    toc.bytes = std::move(bytes);
    return toc;
}

Result<IoStoreToc> readToc(const FileSource& file) {
    std::uint64_t size = file.getSize();
    // A 32-bit target (WebAssembly) cannot hold a table of contents above 4 GiB in memory.
    if (size > SIZE_MAX) return fail(ErrorCode::InvalidData, "readToc.size");
    auto bytes = std::make_unique_for_overwrite<std::byte[]>(static_cast<std::size_t>(size));
    Result<void> read = file.read(0, {bytes.get(), static_cast<std::size_t>(size)});
    if (!read) return std::unexpected(read.error());
    return parseToc(std::move(bytes), size, file.getName());
}

IoStoreContainer::IoStoreContainer(IoStoreToc toc, std::vector<std::unique_ptr<FileSource>> partitions,
                                   std::vector<const Decompressor*> decompressors, const Decryptor* decryptor)
    : toc(std::move(toc)), partitions(std::move(partitions)), decompressors(std::move(decompressors)),
      decryptor(decryptor) {
    UE5PARSE_CHECK(this->partitions.size() == this->toc.getPartitionCount());
    UE5PARSE_CHECK(this->decompressors.size() == this->toc.getCompressionMethodCount());
}

IoStoreContainer::~IoStoreContainer() { forgetDecodedBlocks(); }

// Reads the stored bytes of one block (aligned to 16 when encrypted) and decrypts them in place.
namespace {

// The compression blocks of a container, numbered as in the TOC.
class IoStoreBlocks final : public BlockList {
public:
    IoStoreBlocks(const IoStoreToc& toc, std::span<const std::unique_ptr<FileSource>> partitions,
                  std::span<const Decompressor* const> decompressors, const Decryptor* decryptor)
        : toc(toc), partitions(partitions), decompressors(decompressors), decryptor(decryptor) {}

    Result<StoredBlock> getBlock(std::size_t index) const override {
        if (index >= toc.getBlockCount()) return fail(ErrorCode::InvalidData, "IoStoreContainer.read.blocks", index);
        FIoStoreTocCompressedBlockEntry entry = toc.getBlock(index);
        if (entry.compressionMethodIndex >= decompressors.size())
            return fail(ErrorCode::UnknownCompression, "IoStoreContainer.read.method", entry.offset);
        std::uint64_t partition = entry.offset / toc.getPartitionSize();
        if (partition >= partitions.size()) return fail(ErrorCode::InvalidData, "IoStoreContainer.partition", entry.offset);
        if (toc.isEncrypted() && decryptor == nullptr) return fail(ErrorCode::MissingKey, "IoStoreContainer.decryptor", entry.offset);
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
        if (request.method == EncryptionMethod::AesCtr) std::memcpy(request.iv.data(), toc.getIv(index).data(), 12);
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

Result<void> IoStoreContainer::read(std::size_t chunkIndex, std::uint64_t offset, std::span<std::byte> output,
                                    std::stop_token stop) const {
    if (chunkIndex >= toc.getChunkCount()) return fail(ErrorCode::MissingChunk, "IoStoreContainer.read.index");
    FIoOffsetAndLength chunk = toc.getOffsetAndLength(chunkIndex);
    if (offset > chunk.length || output.size() > chunk.length - offset)
        return fail(ErrorCode::ReadPastEnd, "IoStoreContainer.read.range", offset);
    IoStoreBlocks blocks(toc, partitions, decompressors, decryptor);
    return readBlocks(blocks, toc.getCompressionBlockSize(), chunk.offset + offset, output, stop);
}

Result<std::vector<std::byte>> IoStoreContainer::decryptDirectoryIndex() const {
    std::span<const std::byte> stored = toc.getDirectoryIndexBytes();
    std::vector<std::byte> index(stored.begin(), stored.end());
    if (!toc.isEncrypted()) return index;
    if (decryptor == nullptr) return fail(ErrorCode::MissingKey, "IoStoreContainer.directoryIndex");
    DecryptRequest request;
    request.keyGuid = toc.getEncryptionKeyGuid();
    request.method = toc.getEncryptionMethod();
    request.part = EncryptedPart::TocDirectoryIndex;
    request.containerId = toc.getContainerId();
    if (request.method == EncryptionMethod::AesCtr) {
        std::memcpy(request.iv.data(), toc.getIv(toc.getBlockCount()).data(), 12);
        request.data = index;
    } else {
        // ECB works on whole blocks. The stored index is a multiple of 16 bytes.
        if (index.size() % aesBlock != 0) return fail(ErrorCode::InvalidData, "IoStoreContainer.directoryIndex.size");
        request.data = index;
    }
    Result<void> done = decryptor->decrypt(request);
    if (!done) return std::unexpected(done.error());
    return index;
}

}
