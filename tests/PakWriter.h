#pragma once

// Writes small .pak files in memory, in the layout of UE's UnrealPak (FPakEntry::Serialize, FPakFile::EncodePakEntry,
// the primary and full directory indexes, FPakInfo), so that the reader can be tested without game data. Test code
// only.

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "IoStoreWriter.h"

namespace test {

struct PakFileSpec {
    std::string path;      // relative to the mount point
    Bytes data;
    bool encode = true;    // false: a non-encoded entry, as UnrealPak writes when an entry does not fit the bit fields
    bool deleted = false;  // a delete record: the path is in the index with no entry
};

struct PakOptions {
    std::string name = "pakchunk1-Test.pak";
    std::int32_t version = 12;
    std::uint32_t blockSize = 4096;
    bool compress = true;
    std::optional<ue5parse::AesKey> key;   // encrypts the index and the entries
    ue5parse::FGuid keyGuid;
    std::string mountPoint = "../../../Game/Content/";
};

namespace pak {

struct Entry {
    std::uint64_t offset = 0, size = 0, uncompressedSize = 0;
    std::uint32_t method = 0, blockSize = 0;
    bool encrypted = false;
    std::vector<std::pair<std::uint64_t, std::uint64_t>> blocks;   // relative to offset, as serialized
};

inline void putHeader(Writer& out, const Entry& entry) {
    out.put<std::uint64_t>(entry.offset);
    out.put<std::uint64_t>(entry.size);
    out.put<std::uint64_t>(entry.uncompressedSize);
    out.put<std::uint32_t>(entry.method);
    for (int i = 0; i < 20; i++) out.putU8(0);
    if (entry.method != 0) {
        out.put<std::int32_t>(static_cast<std::int32_t>(entry.blocks.size()));
        for (const auto& [start, end] : entry.blocks) {
            out.put<std::uint64_t>(start);
            out.put<std::uint64_t>(end);
        }
    }
    out.putU8(entry.encrypted ? 1 : 0);
    out.put<std::uint32_t>(entry.blockSize);
}

inline void putEncoded(Writer& out, const Entry& entry) {
    bool offset32 = entry.offset <= 0xFFFFFFFFull;
    bool size32 = entry.size <= 0xFFFFFFFFull;
    bool uncompressed32 = entry.uncompressedSize <= 0xFFFFFFFFull;
    std::uint32_t packedBlockSize = 0;
    if (entry.blocks.size() > 1) {
        packedBlockSize = (entry.blockSize >> 11) & 0x3F;
        if ((packedBlockSize << 11) != entry.blockSize) packedBlockSize = 0x3F;
    }
    std::uint32_t flags = (offset32 ? 1u << 31 : 0) | (uncompressed32 ? 1u << 30 : 0) | (size32 ? 1u << 29 : 0) |
                          (entry.method << 23) | (entry.encrypted ? 1u << 22 : 0) |
                          (static_cast<std::uint32_t>(entry.blocks.size()) << 6) | packedBlockSize;
    out.put<std::uint32_t>(flags);
    if (packedBlockSize == 0x3F) out.put<std::uint32_t>(entry.blockSize);
    if (offset32) out.put<std::uint32_t>(static_cast<std::uint32_t>(entry.offset));
    else out.put<std::uint64_t>(entry.offset);
    if (uncompressed32) out.put<std::uint32_t>(static_cast<std::uint32_t>(entry.uncompressedSize));
    else out.put<std::uint64_t>(entry.uncompressedSize);
    if (entry.method == 0) return;
    if (size32) out.put<std::uint32_t>(static_cast<std::uint32_t>(entry.size));
    else out.put<std::uint64_t>(entry.size);
    if (entry.blocks.size() > 1 || (entry.blocks.size() == 1 && entry.encrypted))
        for (const auto& [start, end] : entry.blocks) out.put<std::uint32_t>(static_cast<std::uint32_t>(end - start));
}

inline void encryptPadded(const PakOptions& options, Bytes& bytes) {
    if (!options.key) return;
    bytes.resize((bytes.size() + 15) / 16 * 16);
    encryptEcb(*options.key, bytes);
}

}

inline Bytes writePak(const PakOptions& options, const std::vector<PakFileSpec>& files) {
    bool encrypted = options.key.has_value();
    Writer out;

    // The data: each entry is its serialized header, then its blocks (each padded to 16 when encrypted).
    std::vector<pak::Entry> entries(files.size());
    for (std::size_t i = 0; i < files.size(); i++) {
        if (files[i].deleted) continue;
        const Bytes& data = files[i].data;
        pak::Entry& entry = entries[i];
        entry.offset = out.bytes.size();
        entry.uncompressedSize = data.size();
        entry.encrypted = encrypted;

        std::vector<Bytes> stored;
        if (options.compress && !data.empty()) {
            for (std::size_t start = 0; start < data.size(); start += options.blockSize) {
                std::size_t size = std::min<std::size_t>(options.blockSize, data.size() - start);
                stored.push_back(compressZlib(std::span(data).subspan(start, size)));
            }
            entry.method = 1;
            entry.blockSize = stored.size() == 1 ? static_cast<std::uint32_t>(data.size()) : options.blockSize;
        }

        Writer header;
        entry.blocks.assign(stored.size(), {0, 0});
        pak::putHeader(header, entry);   // only for its size
        std::uint64_t position = header.bytes.size();
        for (std::size_t b = 0; b < stored.size(); b++) {
            entry.blocks[b] = {position, position + stored[b].size()};
            entry.size += stored[b].size();
            position += encrypted ? (stored[b].size() + 15) / 16 * 16 : stored[b].size();
        }
        if (entry.method == 0) entry.size = data.size();

        pak::putHeader(out, entry);
        if (entry.method == 0) {
            Bytes raw = data;
            pak::encryptPadded(options, raw);
            out.putBytes(raw.data(), raw.size());
        } else {
            for (Bytes& block : stored) {
                pak::encryptPadded(options, block);
                out.putBytes(block.data(), block.size());
            }
        }
    }

    // The entries of the primary index, and the location of each file.
    Writer encoded;
    std::vector<std::size_t> nonEncoded;
    std::vector<std::int32_t> locations(files.size());
    for (std::size_t i = 0; i < files.size(); i++) {
        if (files[i].deleted) {
            locations[i] = INT32_MIN;
        } else if (files[i].encode) {
            locations[i] = static_cast<std::int32_t>(encoded.bytes.size());
            pak::putEncoded(encoded, entries[i]);
        } else {
            locations[i] = -static_cast<std::int32_t>(nonEncoded.size()) - 1;
            nonEncoded.push_back(i);
        }
    }

    // The full directory index: "/" for the root, "Maps/Ascent/" below it, and every parent directory, even with no
    // files of its own.
    std::map<std::string, std::vector<std::size_t>> directories;
    directories["/"];
    for (std::size_t i = 0; i < files.size(); i++) {
        std::size_t slash = files[i].path.rfind('/');
        std::string directory = slash == std::string::npos ? "/" : files[i].path.substr(0, slash + 1);
        directories[directory].push_back(i);
        for (std::size_t parent = directory.find('/'); parent + 1 < directory.size(); parent = directory.find('/', parent + 1))
            directories[directory.substr(0, parent + 1)];
    }
    Writer directoryIndex;
    directoryIndex.put<std::int32_t>(static_cast<std::int32_t>(directories.size()));
    for (const auto& [directory, members] : directories) {
        directoryIndex.putString(directory);
        directoryIndex.put<std::int32_t>(static_cast<std::int32_t>(members.size()));
        for (std::size_t i : members) {
            std::size_t slash = files[i].path.rfind('/');
            std::string name = slash == std::string::npos ? files[i].path : files[i].path.substr(slash + 1);
            if (options.version >= 12) {
                directoryIndex.put<std::int32_t>(static_cast<std::int32_t>(name.size()));
                directoryIndex.putBytes(name.data(), name.size());
            } else {
                directoryIndex.putString(name);
            }
            directoryIndex.put<std::int32_t>(locations[i]);
        }
    }
    pak::encryptPadded(options, directoryIndex.bytes);
    std::uint64_t directoryOffset = out.bytes.size();
    out.putBytes(directoryIndex.bytes.data(), directoryIndex.bytes.size());

    Writer primary;
    primary.putString(options.mountPoint);
    primary.put<std::int32_t>(static_cast<std::int32_t>(files.size()));
    primary.put<std::uint64_t>(0x1234);   // PathHashSeed
    primary.put<std::uint32_t>(1);        // a path hash index; the reader skips it
    primary.put<std::int64_t>(0);
    primary.put<std::int64_t>(0);
    for (int i = 0; i < 20; i++) primary.putU8(0);
    primary.put<std::uint32_t>(1);
    primary.put<std::int64_t>(static_cast<std::int64_t>(directoryOffset));
    primary.put<std::int64_t>(static_cast<std::int64_t>(directoryIndex.bytes.size()));
    for (int i = 0; i < 20; i++) primary.putU8(0);
    primary.put<std::int32_t>(static_cast<std::int32_t>(encoded.bytes.size()));
    primary.putBytes(encoded.bytes.data(), encoded.bytes.size());
    primary.put<std::int32_t>(static_cast<std::int32_t>(nonEncoded.size()));
    for (std::size_t i : nonEncoded) pak::putHeader(primary, entries[i]);
    pak::encryptPadded(options, primary.bytes);
    std::uint64_t indexOffset = out.bytes.size();
    out.putBytes(primary.bytes.data(), primary.bytes.size());

    out.put(options.keyGuid);
    out.putU8(encrypted ? 1 : 0);
    out.put<std::uint32_t>(0x5A6F12E1);
    out.put<std::int32_t>(options.version);
    out.put<std::int64_t>(static_cast<std::int64_t>(indexOffset));
    out.put<std::int64_t>(static_cast<std::int64_t>(primary.bytes.size()));
    for (int i = 0; i < 20; i++) out.putU8(0);
    char names[5][32] = {};
    if (options.compress) std::memcpy(names[0], "Zlib", 4);
    out.putBytes(names, sizeof(names));
    return out.bytes;
}

inline void addPak(const PakOptions& options, const std::vector<PakFileSpec>& files,
                   std::vector<std::unique_ptr<ue5parse::FileSource>>& sources, bool mapped = true) {
    sources.push_back(std::make_unique<MemoryFile>(options.name, writePak(options, files), mapped));
}

}
