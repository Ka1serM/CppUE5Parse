#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <string_view>
#include <vector>

#include "ue5parse/compress/Decompressor.h"
#include "ue5parse/core/Arena.h"
#include "ue5parse/core/Result.h"
#include "ue5parse/crypto/Decryptor.h"
#include "ue5parse/io/FileSource.h"
#include "ue5parse/vfs/IoStore.h"
#include "ue5parse/vfs/Pak.h"

namespace ue5parse {

enum class ArchiveKind : std::uint8_t { IoStore, Pak };

// A container that problems kept from mounting. The other containers mount.
struct ArchiveProblem {
    std::string_view name;   // the .utoc or .pak file name
    Error error;             // MissingKey: locked, the decryptor has no key for its guid
};

// A file of a directory index: its directory, its name, and its chunk (IoStore) or entry (pak).
struct FileEntry {
    std::uint32_t archive;
    std::uint32_t directory;   // in the tree of its archive
    std::uint32_t chunk;       // IoStore: the chunk index. Pak: the entry index.
    std::string_view name;
};

// The directory index of one archive. It stores names, not paths (design §3a): a path is built only on request.
struct DirectoryTree {
    std::string_view mountPoint;                   // as CUE4Parse shows it: "ShooterGame/Content/", or empty
    std::vector<std::uint32_t> parents;            // for each directory: its parent, or ~0u for the root
    std::vector<std::string_view> names;           // for each directory: its name, empty for the root
    std::vector<FileEntry> files;
    std::vector<std::uint64_t> pathHashes;         // for each file: hashPath of its full path
};

// One mounted .utoc/.ucas container or .pak file.
struct MountedArchive {
    std::unique_ptr<Arena> storage;   // the texts of name, the mount point and the tree
    std::string_view name;
    ArchiveKind kind = ArchiveKind::IoStore;
    std::int64_t priority = 0;        // CUE4Parse's ReadOrder: 3, plus 100 * (version + 1) for a _P patch
    std::unique_ptr<IoStoreContainer> ioStore;   // set for IoStore
    std::unique_ptr<PakFile> pak;                // set for Pak
    DirectoryTree tree;
};

// All the fallible work of a mount, before the lookup tables. Owns the archives. The decryptor and the codecs come
// first, because the containers point to them.
struct ArchiveSet {
    std::unique_ptr<Decryptor> decryptor;
    std::unique_ptr<Codecs> codecs;
    std::unique_ptr<Arena> problemNames;
    std::vector<ArchiveProblem> problems;
    std::vector<MountedArchive> archives;
};

// Parses and checks every TOC and pak index, in parallel. sources are all files of a Paks folder: .utoc, .ucas (and
// partitions "_s1.ucas"), .pak. Other files are ignored. A container that cannot mount goes into the problems; only a
// cancellation fails the whole call. decryptor may be null when no container is encrypted.
Result<ArchiveSet> readArchives(std::vector<std::unique_ptr<FileSource>> sources, std::unique_ptr<Decryptor> decryptor,
                                std::unique_ptr<Codecs> codecs, std::stop_token stop = {});

// The path hash that FileSystem::findFile uses: the components between '/' combined from the root, ignoring ASCII
// case. Empty components do not count, so "/A//b" and "a/B" are the same path.
std::uint64_t hashPath(std::string_view path);

struct ChunkLocation {
    std::uint32_t archive;
    std::uint32_t chunk;
};

// The mounted files and chunks of all archives, with the lookup tables. Immutable; all methods are thread-safe.
class FileSystem {
public:
    // Builds the chunk and path tables in parallel; cannot fail.
    explicit FileSystem(ArchiveSet archives);
    FileSystem(FileSystem&&) noexcept;
    FileSystem& operator=(FileSystem&&) noexcept;
    ~FileSystem();

    std::size_t getArchiveCount() const { return set.archives.size(); }
    const MountedArchive& getArchive(std::size_t index) const { return set.archives[index]; }
    std::span<const ArchiveProblem> getProblems() const { return set.problems; }
    const Codecs& getCodecs() const { return *set.codecs; }

    // Files: the merged view, where a patch hides the base file of the same path. The order has no meaning.
    std::span<const FileEntry> getFiles() const { return files; }
    // O(path length). The path as CUE4Parse shows it: "ShooterGame/Content/Maps/Ascent.umap". Ignores case.
    std::optional<FileEntry> findFile(std::string_view path) const;
    // The full path of a file into buffer. Returns the used part; empty when the buffer is too small.
    std::string_view writePath(const FileEntry& file, std::span<char> buffer) const;
    std::uint64_t getFileSize(const FileEntry& file) const;
    Result<void> readFile(const FileEntry& file, std::uint64_t offset, std::span<std::byte> output,
                          std::stop_token stop = {}) const;

    // Chunks: O(1). The highest priority archive wins.
    std::optional<ChunkLocation> findChunk(const FIoChunkId& id) const;
    std::uint64_t getChunkSize(ChunkLocation location) const;
    Result<void> readChunk(ChunkLocation location, std::uint64_t offset, std::span<std::byte> output,
                           std::stop_token stop = {}) const;
    Result<std::vector<std::byte>> readChunk(const FIoChunkId& id, std::stop_token stop = {}) const;

private:
    struct Tables;
    ArchiveSet set;
    std::vector<FileEntry> files;
    std::unique_ptr<Tables> tables;
};

}
