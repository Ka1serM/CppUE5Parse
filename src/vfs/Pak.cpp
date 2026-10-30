#include <algorithm>
#include <cstring>

#include "ue5parse/core/Check.h"
#include "ue5parse/vfs/Pak.h"

#include "BlockReader.h"

namespace ue5parse {

namespace {

constexpr std::uint8_t flagEncrypted = 0x01;
// FPakEntry::GetSerializedSize without the blocks: Offset, Size, UncompressedSize, CompressionMethodIndex, Hash,
// Flags, CompressionBlockSize.
constexpr std::uint64_t entryHeaderSize = 8 * 3 + 4 + 20 + 1 + 4;
// Offsets and sizes above this are corrupt. It keeps every sum of them below 2^64.
constexpr std::uint64_t maxOffset = 1ull << 62;

// The footer of an older or newer layout, to tell an unsupported version from a file that is not a pak.
bool hasMagicAt(std::span<const std::byte> tail, std::size_t footerSize, std::size_t magicOffset) {
    if (tail.size() < footerSize) return false;
    std::uint32_t found;
    std::memcpy(&found, tail.data() + tail.size() - footerSize + magicOffset, 4);
    return found == FPakInfo::magic;
}

// The block layout of a compressed entry must cover its uncompressed size exactly, so that a read can index the
// blocks by offset / block size.
bool hasValidBlocks(const FPakEntry& entry) {
    if (entry.offset >= maxOffset || entry.size >= maxOffset || entry.uncompressedSize >= maxOffset) return false;
    if (entry.compressionMethodIndex == 0 || entry.uncompressedSize == 0) return true;
    if (entry.compressionBlockSize == 0) return false;
    return entry.blockCount == (entry.uncompressedSize + entry.compressionBlockSize - 1) / entry.compressionBlockSize;
}

}

std::string_view FPakInfo::getMethodName(std::size_t index) const {
    return {methodNames[index], strnlen(methodNames[index], methodNameLength)};
}

Result<FPakInfo> parsePakInfo(std::span<const std::byte> tail) {
    if (!hasMagicAt(tail, FPakInfo::serializedSize, 17)) {
        // FrozenIndex adds a byte; versions before FNameBasedCompressionMethod have no method names, and versions
        // before EncryptionKeyGuid no guid.
        bool older = hasMagicAt(tail, FPakInfo::serializedSize + 1, 17) || hasMagicAt(tail, 61, 17) ||
                     hasMagicAt(tail, 45, 1);
        if (older) return fail(ErrorCode::UnsupportedVersion, "parsePakInfo.version");
        return fail(ErrorCode::BadMagic, "parsePakInfo.magic");
    }

    ByteReader reader{tail.last(FPakInfo::serializedSize)};
    FPakInfo info;
    info.encryptionKeyGuid = reader.read<FGuid>();
    info.encryptedIndex = reader.read<std::uint8_t>() != 0;
    reader.skip(4);
    std::int32_t version = reader.read<std::int32_t>();
    std::int64_t indexOffset = reader.read<std::int64_t>();
    std::int64_t indexSize = reader.read<std::int64_t>();
    reader.skip(20);   // the SHA-1 of the index; not checked, as in CUE4Parse
    std::span<const std::byte> names = reader.readBytes(FPakInfo::methodNameLength * FPakInfo::maxMethods);
    if (reader.hasFailed()) return std::unexpected(reader.getError());

    if (version < static_cast<std::int32_t>(EPakFileVersion::PathHashIndex) ||
        version > static_cast<std::int32_t>(EPakFileVersion::Utf8PakDirectory))
        return fail(ErrorCode::UnsupportedVersion, "parsePakInfo.version");
    if (indexOffset < 0 || indexSize < 0 || static_cast<std::uint64_t>(indexOffset) >= maxOffset ||
        static_cast<std::uint64_t>(indexSize) >= maxOffset)
        return fail(ErrorCode::InvalidData, "parsePakInfo.index");
    info.version = static_cast<EPakFileVersion>(version);
    info.indexOffset = static_cast<std::uint64_t>(indexOffset);
    info.indexSize = static_cast<std::uint64_t>(indexSize);

    // As FPakInfo::Serialize: empty names are skipped, so the indices of the entries count only the used names, and
    // the last of the 32 bytes is always the terminator.
    for (std::size_t i = 0; i < FPakInfo::maxMethods; i++) {
        const char* name = reinterpret_cast<const char*>(names.data() + i * FPakInfo::methodNameLength);
        if (name[0] == 0) continue;
        std::memcpy(info.methodNames[info.methodCount], name, FPakInfo::methodNameLength - 1);
        info.methodNames[info.methodCount][FPakInfo::methodNameLength - 1] = 0;
        info.methodCount++;
    }
    return info;
}

Result<FPakInfo> readPakInfo(const FileSource& file) {
    std::uint64_t size = file.getSize();
    std::byte tail[FPakInfo::serializedSize + 1];
    std::size_t tailSize = static_cast<std::size_t>(std::min<std::uint64_t>(size, sizeof(tail)));
    Result<void> read = file.read(size - tailSize, {tail, tailSize});
    if (!read) return std::unexpected(read.error());
    Result<FPakInfo> info = parsePakInfo({tail, tailSize});
    if (!info) return info;
    if (info->indexOffset + info->indexSize > size) return fail(ErrorCode::InvalidData, "readPakInfo.index");
    if (info->encryptedIndex && info->indexSize % aesBlock != 0) return fail(ErrorCode::InvalidData, "readPakInfo.indexSize");
    return info;
}

std::uint64_t FPakEntry::getHeaderSize() const {
    if (compressionMethodIndex == 0) return entryHeaderSize;
    return entryHeaderSize + 4 + std::uint64_t(blockCount) * sizeof(FPakCompressedBlock);
}

Result<std::uint32_t> decodePakEntry(std::span<const std::byte> encoded, std::size_t offset, const FPakInfo& info,
                                     PakEntries& out) {
    if (offset > encoded.size()) return fail(ErrorCode::InvalidData, "decodePakEntry.offset", offset);
    ByteReader reader{encoded.subspan(offset)};
    // Bit 31: offset fits 32 bits. 30: uncompressed size fits. 29: size fits. 28-23: method. 22: encrypted.
    // 21-6: block count. 5-0: block size / 2048, or 0x3f when a full uint32 follows.
    std::uint32_t value = reader.read<std::uint32_t>();
    std::uint32_t blockSize = (value & 0x3f) == 0x3f ? reader.read<std::uint32_t>() : (value & 0x3f) << 11;

    FPakEntry entry;
    entry.compressionMethodIndex = static_cast<std::uint8_t>((value >> 23) & 0x3f);
    entry.offset = (value & (1u << 31)) != 0 ? reader.read<std::uint32_t>() : reader.read<std::uint64_t>();
    entry.uncompressedSize = (value & (1u << 30)) != 0 ? reader.read<std::uint32_t>() : reader.read<std::uint64_t>();
    if (entry.compressionMethodIndex != 0)
        entry.size = (value & (1u << 29)) != 0 ? reader.read<std::uint32_t>() : reader.read<std::uint64_t>();
    else
        entry.size = entry.uncompressedSize;
    entry.encrypted = (value & (1u << 22)) != 0;
    if (reader.hasFailed()) return std::unexpected(reader.getError());
    if (entry.compressionMethodIndex >= info.methodCount) return fail(ErrorCode::UnknownCompression, "decodePakEntry.method");

    // The blocks are only stored for compressed entries.
    std::uint32_t blockCount = entry.compressionMethodIndex != 0 ? (value >> 6) & 0xffff : 0;
    entry.blockCount = blockCount;
    if (blockCount == 1) blockSize = entry.uncompressedSize <= ~0u ? static_cast<std::uint32_t>(entry.uncompressedSize) : 0;
    entry.compressionBlockSize = blockCount > 0 ? blockSize : 0;
    if (!hasValidBlocks(entry)) return fail(ErrorCode::InvalidData, "decodePakEntry.blocks", offset);

    entry.firstBlock = static_cast<std::uint32_t>(out.blocks.size());
    std::uint64_t start = entry.offset + entry.getHeaderSize();
    if (blockCount == 1 && !entry.encrypted) {
        out.blocks.push_back({start, start + entry.size});
    } else {
        std::uint64_t alignment = entry.encrypted ? aesBlock : 1;
        for (std::uint32_t i = 0; i < blockCount; i++) {
            std::uint32_t length = reader.read<std::uint32_t>();
            out.blocks.push_back({start, start + length});
            start += alignUp(length, alignment);
        }
        if (reader.hasFailed()) {
            out.blocks.resize(entry.firstBlock);
            return std::unexpected(reader.getError());
        }
    }
    out.entries.push_back(entry);
    return static_cast<std::uint32_t>(out.entries.size() - 1);
}

Result<std::uint32_t> readPakEntry(ByteReader& reader, const FPakInfo& info, PakEntries& out) {
    FPakEntry entry;
    entry.offset = reader.read<std::uint64_t>();
    entry.size = reader.read<std::uint64_t>();
    entry.uncompressedSize = reader.read<std::uint64_t>();
    std::uint32_t method = reader.read<std::uint32_t>();
    reader.skip(20);   // hash
    if (reader.hasFailed()) return std::unexpected(reader.getError());
    if (method >= info.methodCount) return fail(ErrorCode::UnknownCompression, "readPakEntry.method");
    entry.compressionMethodIndex = static_cast<std::uint8_t>(method);

    std::size_t firstBlock = out.blocks.size();
    if (method != 0) {
        std::uint32_t count = reader.readCount(sizeof(FPakCompressedBlock));
        for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
            // Relative to the entry since RelativeChunkOffsets.
            FPakCompressedBlock block = reader.read<FPakCompressedBlock>();
            if (block.start >= maxOffset || block.end >= maxOffset || block.end < block.start) {
                out.blocks.resize(firstBlock);
                return fail(ErrorCode::InvalidData, "readPakEntry.block");
            }
            out.blocks.push_back({entry.offset + block.start, entry.offset + block.end});
        }
        entry.blockCount = count;
    }
    std::uint8_t flags = reader.read<std::uint8_t>();
    entry.compressionBlockSize = reader.read<std::uint32_t>();
    if (reader.hasFailed()) {
        out.blocks.resize(firstBlock);
        return std::unexpected(reader.getError());
    }
    entry.encrypted = (flags & flagEncrypted) != 0;
    entry.firstBlock = static_cast<std::uint32_t>(firstBlock);
    if (!hasValidBlocks(entry)) {
        out.blocks.resize(firstBlock);
        return fail(ErrorCode::InvalidData, "readPakEntry.blocks");
    }
    out.entries.push_back(entry);
    return static_cast<std::uint32_t>(out.entries.size() - 1);
}

Result<PakPrimaryIndex> parsePakPrimaryIndex(std::span<const std::byte> index, const FPakInfo& info) {
    ByteReader reader{index};
    PakPrimaryIndex primary;
    primary.mountPoint = reader.readStringBytes(primary.mountPointWide);
    std::int32_t entryCount = reader.read<std::int32_t>();
    reader.skip(8);   // PathHashSeed
    // bool is 4 bytes in an FArchive.
    if (reader.read<std::uint32_t>() != 0) reader.skip(8 + 8 + 20);   // the path hash index; not used
    bool hasDirectoryIndex = reader.read<std::uint32_t>() != 0;
    std::int64_t directoryOffset = -1;
    std::int64_t directorySize = 0;
    if (hasDirectoryIndex) {
        directoryOffset = reader.read<std::int64_t>();
        directorySize = reader.read<std::int64_t>();
        reader.skip(20);
    }
    std::uint32_t encodedSize = reader.readCount(1);
    primary.encodedEntries = reader.readBytes(encodedSize);
    // Each entry has at least its fixed part.
    std::uint32_t nonEncodedCount = reader.readCount(entryHeaderSize);
    if (reader.hasFailed()) return std::unexpected(reader.getError());
    if (entryCount < 0) return fail(ErrorCode::InvalidData, "parsePakPrimaryIndex.entryCount");
    if (!hasDirectoryIndex || directoryOffset == -1)
        return fail(ErrorCode::UnsupportedVersion, "parsePakPrimaryIndex.directoryIndex");
    if (directoryOffset < 0 || directorySize < 0 || static_cast<std::uint64_t>(directoryOffset) >= maxOffset ||
        static_cast<std::uint64_t>(directorySize) >= maxOffset)
        return fail(ErrorCode::InvalidData, "parsePakPrimaryIndex.directoryIndex");
    primary.entryCount = static_cast<std::uint32_t>(entryCount);
    primary.directoryIndexOffset = static_cast<std::uint64_t>(directoryOffset);
    primary.directoryIndexSize = static_cast<std::uint64_t>(directorySize);

    primary.nonEncoded.entries.reserve(nonEncodedCount);
    for (std::uint32_t i = 0; i < nonEncodedCount; i++) {
        Result<std::uint32_t> entry = readPakEntry(reader, info, primary.nonEncoded);
        if (!entry) return std::unexpected(entry.error());
    }
    return primary;
}

PakFile::PakFile(FPakInfo info, PakEntries entries, std::unique_ptr<FileSource> file,
                 std::vector<const Decompressor*> decompressors, const Decryptor* decryptor)
    : info(info), entries(std::move(entries)), file(std::move(file)), decompressors(std::move(decompressors)),
      decryptor(decryptor) {
    UE5PARSE_CHECK(this->decompressors.size() == this->info.methodCount);
}

PakFile::~PakFile() { forgetDecodedBlocks(); }

// Decrypts stored bytes of an entry, from position in the .pak, in place. raw is a multiple of 16 bytes.
Result<void> PakFile::decrypt(std::uint64_t position, std::span<std::byte> raw) const {
    if (decryptor == nullptr) return fail(ErrorCode::MissingKey, "PakFile.decryptor", position);
    DecryptRequest request;
    request.data = raw;
    request.keyGuid = info.encryptionKeyGuid;
    request.method = EncryptionMethod::AesEcb;
    request.part = EncryptedPart::PakEntry;
    request.offset = position;
    return decryptor->decrypt(request);
}

// The compression blocks of one entry, numbered from its start.
class PakFile::PakEntryBlocks final : public BlockList {
public:
    PakEntryBlocks(const PakFile& pak, const FPakEntry& entry) : pak(pak), entry(entry), blocks(pak.getBlocks(entry)) {}

    Result<StoredBlock> getBlock(std::size_t index) const override {
        if (index >= blocks.size()) return fail(ErrorCode::InvalidData, "PakFile.read.blocks", index);
        const FPakCompressedBlock& stored = blocks[index];
        if (stored.end < stored.start) return fail(ErrorCode::InvalidData, "PakFile.read.block", stored.start);
        if (entry.encrypted && pak.decryptor == nullptr) return fail(ErrorCode::MissingKey, "PakFile.decryptor", stored.start);
        StoredBlock block;
        block.file = pak.file.get();
        block.position = stored.start;
        block.compressedSize = static_cast<std::size_t>(stored.end - stored.start);
        block.storedSize = static_cast<std::size_t>(entry.encrypted ? alignUp(block.compressedSize, aesBlock) : block.compressedSize);
        std::uint64_t blockSize = entry.compressionBlockSize;
        block.uncompressedSize = static_cast<std::size_t>(std::min<std::uint64_t>(blockSize, entry.uncompressedSize - index * blockSize));
        block.decompressor = pak.decompressors[entry.compressionMethodIndex];
        block.encrypted = entry.encrypted;
        return block;
    }

    Result<void> decrypt(std::size_t index, std::span<std::byte> stored) const override {
        return pak.decrypt(blocks[index].start, stored);
    }

private:
    const PakFile& pak;
    const FPakEntry& entry;
    std::span<const FPakCompressedBlock> blocks;
};

Result<void> PakFile::read(std::size_t entryIndex, std::uint64_t offset, std::span<std::byte> output,
                           std::stop_token stop) const {
    if (entryIndex >= entries.entries.size()) return fail(ErrorCode::MissingFile, "PakFile.read.index");
    const FPakEntry& entry = entries.entries[entryIndex];
    if (offset > entry.uncompressedSize || output.size() > entry.uncompressedSize - offset)
        return fail(ErrorCode::ReadPastEnd, "PakFile.read.range", offset);
    if (output.empty()) return {};
    if (stop.stop_requested()) return fail(ErrorCode::Cancelled, "PakFile.read");

    std::uint64_t dataStart = entry.offset + entry.getHeaderSize();
    if (entry.compressionMethodIndex == 0) {
        if (!entry.encrypted) return file->read(dataStart + offset, output);
        // ECB decrypts each 16-byte unit on its own, so only the units of the range are read.
        std::uint64_t first = offset / aesBlock * aesBlock;
        std::uint64_t end = alignUp(offset + output.size(), aesBlock);
        std::vector<std::byte>& raw = threadBuffer(0);
        if (raw.size() < end - first) raw.resize(end - first);
        std::span<std::byte> stored{raw.data(), static_cast<std::size_t>(end - first)};
        Result<void> read = file->read(dataStart + first, stored);
        if (!read) return read;
        Result<void> decrypted = decrypt(dataStart + first, stored);
        if (!decrypted) return decrypted;
        std::memcpy(output.data(), raw.data() + (offset - first), output.size());
        return {};
    }

    PakEntryBlocks blocks(*this, entry);
    return readBlocks(blocks, entry.compressionBlockSize, offset, output, stop);
}

}
