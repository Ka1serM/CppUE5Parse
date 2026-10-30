#pragma once

// Writes small IoStore containers in memory, in the layout of UE's FIoStoreWriter, so that the reader can be tested
// without game data. Test code only.

#include <libdeflate.h>
#include <mbedtls/aes.h>

#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "ue5parse/crypto/Decryptor.h"
#include "ue5parse/io/FileSource.h"
#include "ue5parse/vfs/IoStore.h"

namespace test {

using Bytes = std::vector<std::byte>;

class MemoryFile final : public ue5parse::FileSource {
public:
    MemoryFile(std::string name, Bytes bytes) : name(std::move(name)), bytes(std::move(bytes)) {}
    std::string_view getName() const noexcept override { return name; }
    std::uint64_t getSize() const noexcept override { return bytes.size(); }
    ue5parse::Result<void> read(std::uint64_t offset, std::span<std::byte> buffer) const noexcept override {
        if (offset > bytes.size() || buffer.size() > bytes.size() - offset)
            return ue5parse::fail(ue5parse::ErrorCode::ReadPastEnd, "MemoryFile.read", offset);
        if (!buffer.empty()) std::memcpy(buffer.data(), bytes.data() + offset, buffer.size());
        return {};
    }

private:
    std::string name;
    Bytes bytes;
};

inline Bytes makeData(std::size_t size, std::uint32_t seed, bool compressible) {
    Bytes data(size);
    std::uint32_t state = seed * 2654435761u + 1;
    for (std::size_t i = 0; i < size; i++) {
        state = state * 1664525u + 1013904223u;
        data[i] = static_cast<std::byte>(compressible ? (i / 64 + seed) & 0x0F : state >> 24);
    }
    return data;
}

struct Chunk {
    ue5parse::FIoChunkId id;
    Bytes data;
    std::string path;   // relative to the mount point; empty when the chunk is not a file
};

struct ContainerOptions {
    std::string name = "pakchunk0-Test";   // without the extension
    std::uint32_t blockSize = 4096;
    bool compress = true;
    std::optional<ue5parse::AesKey> key;
    ue5parse::FGuid keyGuid;
    std::uint64_t containerId = 1;
    std::uint64_t partitionSize = 0;   // 0: one partition
    std::string mountPoint = "../../../Game/Content/";
    // 5: UE 5.0-5.3 (PerfectHashWithOverflow), 6: 5.4, 8: 5.5-5.8.
    ue5parse::EIoStoreTocVersion version = ue5parse::EIoStoreTocVersion::ReplaceIoChunkHashWithIoHash;
};

struct Container {
    std::string name;
    Bytes toc;
    std::vector<Bytes> partitions;
};

class Writer {
public:
    void putU8(std::uint8_t value) { bytes.push_back(static_cast<std::byte>(value)); }
    template <class T> void put(T value) {
        std::size_t at = bytes.size();
        bytes.resize(at + sizeof(T));
        std::memcpy(bytes.data() + at, &value, sizeof(T));
    }
    void putBytes(const void* data, std::size_t size) {
        std::size_t at = bytes.size();
        bytes.resize(at + size);
        if (size > 0) std::memcpy(bytes.data() + at, data, size);
    }
    void putString(const std::string& text) {
        put<std::int32_t>(static_cast<std::int32_t>(text.size() + 1));
        putBytes(text.data(), text.size());
        putU8(0);
    }
    void putBigEndian40(std::uint64_t value) {
        for (int i = 4; i >= 0; i--) putU8(static_cast<std::uint8_t>(value >> (i * 8)));
    }

    Bytes bytes;
};

inline void encryptEcb(const ue5parse::AesKey& key, std::span<std::byte> data) {
    mbedtls_aes_context context;
    mbedtls_aes_init(&context);
    mbedtls_aes_setkey_enc(&context, reinterpret_cast<const unsigned char*>(key.data()), 256);
    for (std::size_t offset = 0; offset + 16 <= data.size(); offset += 16) {
        auto* block = reinterpret_cast<unsigned char*>(data.data() + offset);
        mbedtls_aes_crypt_ecb(&context, MBEDTLS_AES_ENCRYPT, block, block);
    }
    mbedtls_aes_free(&context);
}

inline Bytes compressZlib(std::span<const std::byte> input) {
    libdeflate_compressor* compressor = libdeflate_alloc_compressor(6);
    Bytes output(libdeflate_zlib_compress_bound(compressor, input.size()));
    std::size_t size = libdeflate_zlib_compress(compressor, input.data(), input.size(), output.data(), output.size());
    libdeflate_free_compressor(compressor);
    output.resize(size);
    return output;
}

// FIoDirectoryIndexResource for the given paths: file i has user data chunkIndices[i].
inline Bytes writeDirectoryIndex(const std::string& mountPoint, const std::vector<std::string>& paths,
                                 const std::vector<std::uint32_t>& chunkIndices) {
    struct Directory {
        std::uint32_t name = ~0u, firstChild = ~0u, nextSibling = ~0u, firstFile = ~0u;
    };
    struct File {
        std::uint32_t name = ~0u, nextFile = ~0u, userData = 0;
    };
    std::vector<Directory> directories(1);
    std::vector<File> files;
    std::vector<std::string> strings;
    std::map<std::string, std::uint32_t> stringIndex;
    auto intern = [&](const std::string& text) {
        auto [found, inserted] = stringIndex.try_emplace(text, static_cast<std::uint32_t>(strings.size()));
        if (inserted) strings.push_back(text);
        return found->second;
    };

    for (std::size_t i = 0; i < paths.size(); i++) {
        std::uint32_t directory = 0;
        std::size_t start = 0;
        for (std::size_t slash; (slash = paths[i].find('/', start)) != std::string::npos; start = slash + 1) {
            std::uint32_t name = intern(paths[i].substr(start, slash - start));
            std::uint32_t child = directories[directory].firstChild;
            while (child != ~0u && directories[child].name != name) child = directories[child].nextSibling;
            if (child == ~0u) {
                child = static_cast<std::uint32_t>(directories.size());
                Directory entry;
                entry.name = name;
                entry.nextSibling = directories[directory].firstChild;
                directories.push_back(entry);
                directories[directory].firstChild = child;
            }
            directory = child;
        }
        File file;
        file.name = intern(paths[i].substr(start));
        file.nextFile = directories[directory].firstFile;
        file.userData = chunkIndices[i];
        directories[directory].firstFile = static_cast<std::uint32_t>(files.size());
        files.push_back(file);
    }

    Writer writer;
    writer.putString(mountPoint);
    writer.put<std::int32_t>(static_cast<std::int32_t>(directories.size()));
    writer.putBytes(directories.data(), directories.size() * sizeof(Directory));
    writer.put<std::int32_t>(static_cast<std::int32_t>(files.size()));
    writer.putBytes(files.data(), files.size() * sizeof(File));
    writer.put<std::int32_t>(static_cast<std::int32_t>(strings.size()));
    for (const std::string& text : strings) writer.putString(text);
    return writer.bytes;
}

inline Container writeContainer(const ContainerOptions& options, const std::vector<Chunk>& chunks) {
    bool encrypted = options.key.has_value();
    std::uint64_t partitionSize = options.partitionSize != 0 ? options.partitionSize : ~0ull;
    Container container;
    container.name = options.name;
    container.partitions.emplace_back();

    struct Block {
        std::uint64_t offset;
        std::uint32_t compressedSize, uncompressedSize;
        std::uint8_t method;
    };
    std::vector<Block> blocks;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> offsetLengths;
    std::uint64_t virtualOffset = 0;
    for (const Chunk& chunk : chunks) {
        offsetLengths.push_back({virtualOffset, chunk.data.size()});
        for (std::size_t start = 0; start < chunk.data.size(); start += options.blockSize) {
            std::size_t size = std::min<std::size_t>(options.blockSize, chunk.data.size() - start);
            std::span<const std::byte> raw(chunk.data.data() + start, size);
            Bytes stored(raw.begin(), raw.end());
            std::uint8_t method = 0;
            if (options.compress) {
                Bytes compressed = compressZlib(raw);
                if (compressed.size() < size) {
                    stored = std::move(compressed);
                    method = 1;
                }
            }
            std::uint32_t storedSize = static_cast<std::uint32_t>(stored.size());
            if (encrypted) {
                stored.resize((stored.size() + 15) / 16 * 16);
                encryptEcb(*options.key, stored);
            }
            // A block never crosses a partition.
            Bytes* partition = &container.partitions.back();
            if (partition->size() + stored.size() > partitionSize) {
                container.partitions.emplace_back();
                partition = &container.partitions.back();
            }
            std::uint64_t offset = (container.partitions.size() - 1) * (options.partitionSize != 0 ? partitionSize : 0) +
                                   partition->size();
            partition->insert(partition->end(), stored.begin(), stored.end());
            blocks.push_back({offset, storedSize, static_cast<std::uint32_t>(size), method});
        }
        virtualOffset += (chunk.data.size() + options.blockSize - 1) / options.blockSize * options.blockSize;
    }

    std::vector<std::string> paths;
    std::vector<std::uint32_t> pathChunks;
    for (std::size_t i = 0; i < chunks.size(); i++) {
        if (chunks[i].path.empty()) continue;
        paths.push_back(chunks[i].path);
        pathChunks.push_back(static_cast<std::uint32_t>(i));
    }
    Bytes directoryIndex = writeDirectoryIndex(options.mountPoint, paths, pathChunks);
    if (encrypted) {
        directoryIndex.resize((directoryIndex.size() + 15) / 16 * 16);
        encryptEcb(*options.key, directoryIndex);
    }

    constexpr std::uint32_t methodNameLength = 32;
    std::uint8_t flags = 8;   // Indexed
    if (options.compress) flags |= 1;
    if (encrypted) flags |= 2;

    Writer toc;
    toc.putBytes("-==--==--==--==-", 16);
    toc.putU8(static_cast<std::uint8_t>(options.version));
    toc.putU8(0);
    toc.put<std::uint16_t>(0);
    toc.put<std::uint32_t>(144);
    toc.put<std::uint32_t>(static_cast<std::uint32_t>(chunks.size()));
    toc.put<std::uint32_t>(static_cast<std::uint32_t>(blocks.size()));
    toc.put<std::uint32_t>(12);
    toc.put<std::uint32_t>(options.compress ? 1 : 0);
    toc.put<std::uint32_t>(methodNameLength);
    toc.put<std::uint32_t>(options.blockSize);
    toc.put<std::uint32_t>(static_cast<std::uint32_t>(directoryIndex.size()));
    toc.put<std::uint32_t>(static_cast<std::uint32_t>(container.partitions.size()));
    toc.put<std::uint64_t>(options.containerId);
    toc.put(options.keyGuid);
    toc.putU8(flags);
    toc.putU8(0);
    toc.put<std::uint16_t>(0);
    toc.put<std::uint32_t>(0);   // perfect hash seeds
    toc.put<std::uint64_t>(partitionSize);
    toc.put<std::uint32_t>(0);   // chunks without perfect hash
    toc.put<std::uint32_t>(0);
    for (int i = 0; i < 5; i++) toc.put<std::uint64_t>(0);

    for (const Chunk& chunk : chunks) toc.putBytes(chunk.id.bytes, 12);
    for (const auto& [offset, length] : offsetLengths) {
        toc.putBigEndian40(offset);
        toc.putBigEndian40(length);
    }
    for (const Block& block : blocks) {
        toc.put<std::uint64_t>(block.offset | (std::uint64_t(block.compressedSize) << 40));
        toc.put<std::uint32_t>(block.uncompressedSize | (std::uint32_t(block.method) << 24));
    }
    if (options.compress) {
        char name[methodNameLength] = "Zlib";
        toc.putBytes(name, methodNameLength);
    }
    toc.putBytes(directoryIndex.data(), directoryIndex.size());
    container.toc = std::move(toc.bytes);
    return container;
}

inline void addSources(const Container& container, std::vector<std::unique_ptr<ue5parse::FileSource>>& sources) {
    sources.push_back(std::make_unique<MemoryFile>(container.name + ".utoc", container.toc));
    for (std::size_t i = 0; i < container.partitions.size(); i++) {
        std::string name = container.name + (i > 0 ? "_s" + std::to_string(i) : "") + ".ucas";
        sources.push_back(std::make_unique<MemoryFile>(name, container.partitions[i]));
    }
}

}
