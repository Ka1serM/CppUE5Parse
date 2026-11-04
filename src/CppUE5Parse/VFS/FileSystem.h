#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Compress/Decompressor.h"
#include "CppUE5Parse/Core/Arena.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Crypto/Decryptor.h"
#include "CppUE5Parse/IO/FileSource.h"
#include "CppUE5Parse/VFS/IoStore.h"
#include "CppUE5Parse/VFS/Pak.h"

namespace ue5parse {

enum class ArchiveKind : std::uint8_t { IoStore, Pak };

struct ArchiveProblem {
    std::string_view name;
    Error error;
};

struct FileEntry {
    std::uint32_t archive;
    std::uint32_t directory;
    std::uint32_t chunk;
    std::uint32_t name;
};

struct DirectoryTree {
    std::string_view mountPoint;
    std::vector<std::uint32_t> parents;
    std::vector<std::string_view> names;
    std::vector<char> fileNames;
    std::string_view getFileName(std::uint32_t name) const {
        std::uint16_t size;
        std::memcpy(&size, fileNames.data() + name, 2);
        return {fileNames.data() + name + 2, size};
    }
    std::vector<FileEntry> files;
    std::vector<std::uint64_t> pathHashes;
};

struct MountedArchive {
    std::unique_ptr<Arena> storage;
    std::string_view name;
    ArchiveKind kind = ArchiveKind::IoStore;
    std::int64_t priority = 0;
    std::unique_ptr<IoStoreContainer> ioStore;
    std::unique_ptr<PakFile> pak;
    DirectoryTree tree;
};

struct ArchiveSet {
    std::unique_ptr<Decryptor> decryptor;
    std::unique_ptr<Codecs> codecs;
    std::unique_ptr<Arena> problemNames;
    std::vector<ArchiveProblem> problems;
    std::vector<MountedArchive> archives;
};

Result<ArchiveSet> readArchives(std::vector<std::unique_ptr<FileSource>> sources, std::unique_ptr<Decryptor> decryptor,
                                std::unique_ptr<Codecs> codecs, std::stop_token stop = {});

std::uint64_t hashPath(std::string_view path);

struct ChunkLocation {
    std::uint32_t archive;
    std::uint32_t chunk;
};

class FileSystem {
public:
    explicit FileSystem(ArchiveSet archives);
    FileSystem(FileSystem&&) noexcept;
    FileSystem& operator=(FileSystem&&) noexcept;
    ~FileSystem();

    std::size_t getArchiveCount() const { return set.archives.size(); }
    const MountedArchive& getArchive(std::size_t index) const { return set.archives[index]; }
    std::span<const ArchiveProblem> getProblems() const { return set.problems; }
    const Codecs& getCodecs() const { return *set.codecs; }

    std::span<const FileEntry> getFiles() const { return files; }
    std::optional<FileEntry> findFile(std::string_view path) const;
    std::string_view writePath(const FileEntry& file, std::span<char> buffer) const;
    std::string_view getFileName(const FileEntry& file) const { return set.archives[file.archive].tree.getFileName(file.name); }
    std::uint64_t getFileSize(const FileEntry& file) const;
    Result<void> readFile(const FileEntry& file, std::uint64_t offset, std::span<std::byte> output,
                          std::stop_token stop = {}) const;

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
