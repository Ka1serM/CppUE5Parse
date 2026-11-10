#include "CppUE5Parse/VFS/FileSystem.h"

#include <charconv>
#include <climits>
#include <cstdint>
#include <cstring>

#include <ankerl/unordered_dense.h>
#include <city.h>
#include <tbb/parallel_for.h>

#include "CppUE5Parse/Core/ByteReader.h"
#include "CppUE5Parse/Core/Check.h"
#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Text.h"

namespace ue5parse {

namespace {

constexpr std::uint32_t none = ~0u;

char toLowerAscii(char c) {
    return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
}

bool endsWithIgnoreCase(std::string_view text, std::string_view suffix) {
    if (text.size() < suffix.size())
        return false;
    std::string_view end = text.substr(text.size() - suffix.size());
    for (std::size_t i = 0; i < suffix.size(); i++)
        if (toLowerAscii(end[i]) != toLowerAscii(suffix[i]))
            return false;
    return true;
}

std::uint64_t combinePath(std::uint64_t parent, std::uint64_t componentHash) {
    return Hash128to64(uint128(parent, componentHash));
}

std::int64_t getReadOrder(std::string_view name) {
    std::int64_t order = 3;
    if (!endsWithIgnoreCase(name, "_P.pak") && !endsWithIgnoreCase(name, "_P.utoc") && !endsWithIgnoreCase(name, "_P.o.utoc"))
        return order;
    std::int64_t version = 1;
    std::size_t end = name.rfind('_');
    if (end != std::string_view::npos && end > 0) {
        std::size_t start = name.rfind('_', end - 1);
        if (start != std::string_view::npos) {
            std::int64_t parsed = 0;
            const char* first = name.data() + start + 1;
            const char* last = name.data() + end;
            std::from_chars_result result = std::from_chars(first, last, parsed);
            if (result.ec == std::errc{} && result.ptr == last && parsed >= 1)
                version = parsed + 1;
        }
    }
    return order + 100 * version;
}

std::string_view normalizeMountPoint(std::string_view mountPoint) {
    std::string_view prefix = "../../..";
    std::size_t found = mountPoint.find(prefix);
    bool bad = found != 0;
    if (found != std::string_view::npos)
        mountPoint.remove_prefix(found + prefix.size());
    if (mountPoint.empty() || mountPoint[0] != '/' || (mountPoint.size() > 1 && mountPoint[1] == '.'))
        bad = true;
    if (bad)
        return {};
    return mountPoint.substr(1);
}

std::string_view readUtf8String(ByteReader& reader, Arena& storage) {
    bool wide = false;
    std::span<const std::byte> bytes = reader.readStringBytes(wide);
    return StringView(bytes, wide).toUtf8(storage);
}

bool hasValidMountPoint(std::span<const std::byte> index) {
    constexpr std::int32_t maxLength = 128;
    ByteReader reader{index};
    std::int32_t length = reader.read<std::int32_t>();
    if (reader.hasFailed() || length > maxLength || length < -maxLength)
        return false;
    if (length == 0)
        return reader.read<std::uint8_t>() == 0 && !reader.hasFailed();
    if (length < 0) {
        reader.skip(static_cast<std::size_t>(-length - 1) * 2);
        return reader.read<std::uint16_t>() == 0 && !reader.hasFailed();
    }
    reader.skip(static_cast<std::size_t>(length - 1));
    return reader.read<std::uint8_t>() == 0 && !reader.hasFailed();
}

Result<std::uint32_t> addFileName(DirectoryTree& tree, std::string_view name) {
    if (name.size() > UINT16_MAX || tree.fileNames.size() + 2 + name.size() > UINT32_MAX)
        return fail(ErrorCode::InvalidData, "addFileName.size");
    std::uint32_t at = static_cast<std::uint32_t>(tree.fileNames.size());
    std::uint16_t size = static_cast<std::uint16_t>(name.size());
    tree.fileNames.resize(at + 2 + name.size());
    std::memcpy(tree.fileNames.data() + at, &size, 2);
    std::memcpy(tree.fileNames.data() + at + 2, name.data(), name.size());
    return at;
}

Result<void> parseDirectoryIndex(std::span<const std::byte> stored, std::size_t chunkCount, Arena& storage, DirectoryTree& tree) {
    ByteReader reader{stored};
    tree.mountPoint = normalizeMountPoint(storage.copy(readUtf8String(reader, storage)));
    std::vector<FIoDirectoryIndexEntry> directories(reader.readCount(sizeof(FIoDirectoryIndexEntry)));
    reader.readInto(std::span(directories));
    std::vector<FIoFileIndexEntry> files(reader.readCount(sizeof(FIoFileIndexEntry)));
    reader.readInto(std::span(files));
    std::vector<std::string_view> strings(reader.readCount(4));
    for (std::size_t i = 0; i < strings.size() && !reader.hasFailed(); i++)
        strings[i] = readUtf8String(reader, storage);
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    std::vector<std::uint32_t> fileNameOf(strings.size(), none);
    std::vector<std::uint64_t> stringHashes(strings.size());
    for (std::size_t i = 0; i < strings.size(); i++)
        stringHashes[i] = hashName(strings[i]);
    if (directories.empty())
        return {};

    auto getString = [&](std::uint32_t name, std::string_view& text) {
        if (name == none) {
            text = {};
            return true;
        }
        if (name >= strings.size())
            return false;
        text = strings[name];
        return true;
    };

    tree.parents.assign(directories.size(), none);
    tree.names.assign(directories.size(), {});
    std::vector<std::uint64_t> directoryHashes(directories.size(), 0);
    std::vector<std::uint8_t> visitedDirectories(directories.size(), 0);
    std::vector<std::uint8_t> visitedFiles(files.size(), 0);
    tree.files.reserve(files.size());
    tree.pathHashes.reserve(files.size());

    if (!getString(directories[0].name, tree.names[0]))
        return fail(ErrorCode::InvalidData, "parseDirectoryIndex.name");
    tree.names[0] = storage.copy(tree.names[0]);
    directoryHashes[0] = hashPath(tree.mountPoint);
    if (!tree.names[0].empty())
        directoryHashes[0] = combinePath(directoryHashes[0], stringHashes[directories[0].name]);
    visitedDirectories[0] = 1;
    std::vector<std::uint32_t> stack{0};

    while (!stack.empty()) {
        std::uint32_t directory = stack.back();
        stack.pop_back();
        const FIoDirectoryIndexEntry& entry = directories[directory];

        for (std::uint32_t file = entry.firstFile; file != none; file = files[file].nextFile) {
            if (file >= files.size() || visitedFiles[file])
                return fail(ErrorCode::InvalidData, "parseDirectoryIndex.file");
            visitedFiles[file] = 1;
            std::string_view name;
            if (!getString(files[file].name, name) || name.empty())
                return fail(ErrorCode::InvalidData, "parseDirectoryIndex.fileName");
            if (files[file].userData >= chunkCount)
                return fail(ErrorCode::InvalidData, "parseDirectoryIndex.chunk");
            std::uint32_t& kept = fileNameOf[files[file].name];
            if (kept == none) {
                Result<std::uint32_t> added = addFileName(tree, name);
                if (!added)
                    return std::unexpected(added.error());
                kept = *added;
            }
            tree.files.push_back(FileEntry{0, directory, files[file].userData, kept});
            tree.pathHashes.push_back(combinePath(directoryHashes[directory], stringHashes[files[file].name]));
        }

        for (std::uint32_t child = entry.firstChild; child != none; child = directories[child].nextSibling) {
            if (child >= directories.size() || visitedDirectories[child])
                return fail(ErrorCode::InvalidData, "parseDirectoryIndex.directory");
            visitedDirectories[child] = 1;
            tree.parents[child] = directory;
            if (!getString(directories[child].name, tree.names[child]))
                return fail(ErrorCode::InvalidData, "parseDirectoryIndex.name");
            tree.names[child] = storage.copy(tree.names[child]);
            directoryHashes[child] =
                tree.names[child].empty() ? directoryHashes[directory] : combinePath(directoryHashes[directory], stringHashes[directories[child].name]);
            stack.push_back(child);
        }
    }
    tree.fileNames.shrink_to_fit();
    return {};
}

class SourceIndex {
public:
    void add(std::string_view name, std::size_t source) { sources.try_emplace(hashName(name), source); }
    bool contains(std::string_view name) const { return sources.contains(hashName(name)); }
    std::optional<std::size_t> find(std::string_view name) const {
        auto found = sources.find(hashName(name));
        if (found == sources.end())
            return std::nullopt;
        return found->second;
    }

private:
    ankerl::unordered_dense::map<std::uint64_t, std::size_t> sources;
};

std::string_view writePartitionName(std::string_view base, std::uint32_t partition, std::span<char> buffer) {
    constexpr std::size_t maxSuffix = 17;
    if (base.size() + maxSuffix > buffer.size())
        return {};
    std::memcpy(buffer.data(), base.data(), base.size());
    std::size_t size = base.size();
    if (partition > 0) {
        buffer[size++] = '_';
        buffer[size++] = 's';
        std::to_chars_result result = std::to_chars(buffer.data() + size, buffer.data() + buffer.size(), partition);
        size = static_cast<std::size_t>(result.ptr - buffer.data());
    }
    std::memcpy(buffer.data() + size, ".ucas", 5);
    return {buffer.data(), size + 5};
}

struct MountInput {
    std::vector<std::unique_ptr<FileSource>>& sources;
    const SourceIndex& index;
    const Decryptor* decryptor;
    const Codecs& codecs;
};

Result<MountedArchive> mountIoStore(const FileSource& tocFile, MountInput& input) {
    Result<IoStoreToc> toc = readToc(tocFile);
    if (!toc)
        return std::unexpected(toc.error());

    std::string_view tocName = tocFile.getName();
    std::string_view base = tocName.substr(0, tocName.size() - std::string_view(".utoc").size());
    std::vector<std::unique_ptr<FileSource>> partitions;
    for (std::uint32_t i = 0; i < toc->getPartitionCount(); i++) {
        char buffer[1024];
        std::optional<std::size_t> source = input.index.find(writePartitionName(base, i, buffer));
        if (!source || input.sources[*source] == nullptr)
            return fail(ErrorCode::MissingFile, "readArchives.ucas");
        partitions.push_back(std::move(input.sources[*source]));
    }

    std::vector<const Decompressor*> decompressors(toc->getCompressionMethodCount(), nullptr);
    for (std::size_t i = 1; i < decompressors.size(); i++) {
        decompressors[i] = input.codecs.find(FName(toc->getCompressionMethodName(i)));
        if (decompressors[i] == nullptr)
            return fail(ErrorCode::UnknownCompression, "readArchives.method");
    }

    if (toc->isEncrypted() && (input.decryptor == nullptr || !input.decryptor->hasKey(toc->getEncryptionKeyGuid())))
        return fail(ErrorCode::MissingKey, "readArchives.key");

    MountedArchive archive;
    archive.storage = std::make_unique<Arena>();
    archive.name = archive.storage->copy(tocName);
    archive.kind = ArchiveKind::IoStore;
    archive.priority = getReadOrder(tocName);
    archive.ioStore = std::make_unique<IoStoreContainer>(std::move(*toc), std::move(partitions), std::move(decompressors), input.decryptor);

    const IoStoreToc& parsed = archive.ioStore->getToc();
    if (!parsed.isIndexed() || !parsed.hasDirectoryIndex())
        return archive;
    Result<std::vector<std::byte>> index = archive.ioStore->takeDirectoryIndex();
    if (!index)
        return std::unexpected(index.error());
    if (!hasValidMountPoint(*index)) {
        if (parsed.isEncrypted())
            return fail(ErrorCode::BadAesKey, "readArchives.mountPoint");
        return fail(ErrorCode::InvalidData, "readArchives.mountPoint");
    }
    Result<void> tree = parseDirectoryIndex(*index, parsed.getChunkCount(), *archive.storage, archive.tree);
    if (!tree)
        return std::unexpected(tree.error());
    return archive;
}

std::string_view readUtf8Name(ByteReader& reader) {
    std::span<const std::byte> bytes = reader.readBytes(reader.readCount(1));
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

Result<void> parsePakDirectoryIndex(std::span<const std::byte> index, const FPakInfo& info, const PakPrimaryIndex& primary, Arena& storage, DirectoryTree& tree,
                                    PakEntries& entries) {
    ByteReader reader{index};
    bool utf8Names = info.version >= EPakFileVersion::Utf8PakDirectory;
    tree.parents.assign(1, none);
    tree.names.assign(1, {});
    std::vector<std::uint64_t> directoryHashes{hashPath(tree.mountPoint)};
    ankerl::unordered_dense::map<std::uint64_t, std::uint32_t> directoryByHash;
    directoryByHash.emplace(directoryHashes[0], 0);

    std::uint32_t directoryCount = reader.readCount(8);
    for (std::uint32_t i = 0; i < directoryCount && !reader.hasFailed(); i++) {
        std::string_view path = readUtf8String(reader, storage);
        std::uint32_t directory = 0;
        for (std::size_t start = 0; start < path.size();) {
            std::size_t end = path.find('/', start);
            if (end == std::string_view::npos)
                end = path.size();
            if (end > start) {
                std::string_view component = path.substr(start, end - start);
                std::uint64_t hash = combinePath(directoryHashes[directory], hashName(component));
                auto [found, inserted] = directoryByHash.try_emplace(hash, static_cast<std::uint32_t>(tree.names.size()));
                if (inserted) {
                    tree.parents.push_back(directory);
                    tree.names.push_back(storage.copy(component));
                    directoryHashes.push_back(hash);
                }
                directory = found->second;
            }
            start = end + 1;
        }

        std::uint32_t fileCount = reader.readCount(8);
        for (std::uint32_t j = 0; j < fileCount && !reader.hasFailed(); j++) {
            std::string_view name = utf8Names ? readUtf8Name(reader) : readUtf8String(reader, storage);
            std::int32_t location = reader.read<std::int32_t>();
            if (reader.hasFailed())
                break;
            if (location == INT32_MIN)
                continue;
            if (name.empty())
                return fail(ErrorCode::InvalidData, "parsePakDirectoryIndex.fileName");

            Result<std::uint32_t> entry;
            if (location >= 0) {
                entry = decodePakEntry(primary.encodedEntries, static_cast<std::size_t>(location), info, entries);
            } else {
                std::size_t nonEncoded = static_cast<std::size_t>(-(location + 1));
                if (nonEncoded >= primary.nonEncoded.entries.size())
                    return fail(ErrorCode::InvalidData, "parsePakDirectoryIndex.location");
                FPakEntry copy = primary.nonEncoded.entries[nonEncoded];
                std::span<const FPakCompressedBlock> blocks = std::span(primary.nonEncoded.blocks).subspan(copy.firstBlock, copy.blockCount);
                copy.firstBlock = static_cast<std::uint32_t>(entries.blocks.size());
                entries.blocks.insert(entries.blocks.end(), blocks.begin(), blocks.end());
                entries.entries.push_back(copy);
                entry = static_cast<std::uint32_t>(entries.entries.size() - 1);
            }
            if (!entry)
                return std::unexpected(entry.error());
            Result<std::uint32_t> kept = addFileName(tree, name);
            if (!kept)
                return std::unexpected(kept.error());
            tree.files.push_back(FileEntry{0, directory, *entry, *kept});
            tree.pathHashes.push_back(combinePath(directoryHashes[directory], hashName(name)));
        }
    }
    tree.fileNames.shrink_to_fit();
    return reader.getResult();
}

Result<std::vector<std::byte>> readPakIndex(const FileSource& file, const FPakInfo& info, std::uint64_t offset, std::uint64_t size,
                                            const Decryptor* decryptor) {
    if (offset > file.getSize() || size > file.getSize() - offset || size > SIZE_MAX)
        return fail(ErrorCode::InvalidData, "readPakIndex.range", offset);
    if (info.encryptedIndex && size % 16 != 0)
        return fail(ErrorCode::InvalidData, "readPakIndex.size", offset);
    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    Result<void> read = file.read(offset, bytes);
    if (!read)
        return std::unexpected(read.error());
    if (!info.encryptedIndex)
        return bytes;
    DecryptRequest request;
    request.data = bytes;
    request.keyGuid = info.encryptionKeyGuid;
    request.method = EncryptionMethod::AesEcb;
    request.part = EncryptedPart::PakIndex;
    request.offset = offset;
    Result<void> done = decryptor->decrypt(request);
    if (!done)
        return std::unexpected(done.error());
    return bytes;
}

Result<MountedArchive> mountPak(std::unique_ptr<FileSource>& source, MountInput& input) {
    const FileSource& file = *source;
    Result<FPakInfo> info = readPakInfo(file);
    if (!info)
        return std::unexpected(info.error());

    std::vector<const Decompressor*> decompressors(info->methodCount, nullptr);
    for (std::size_t i = 1; i < decompressors.size(); i++) {
        decompressors[i] = input.codecs.find(FName(info->getMethodName(i)));
        if (decompressors[i] == nullptr)
            return fail(ErrorCode::UnknownCompression, "readArchives.method");
    }
    if (info->encryptedIndex && (input.decryptor == nullptr || !input.decryptor->hasKey(info->encryptionKeyGuid)))
        return fail(ErrorCode::MissingKey, "readArchives.key");

    Result<std::vector<std::byte>> primaryBytes = readPakIndex(file, *info, info->indexOffset, info->indexSize, input.decryptor);
    if (!primaryBytes)
        return std::unexpected(primaryBytes.error());
    if (!hasValidMountPoint(*primaryBytes)) {
        if (info->encryptedIndex)
            return fail(ErrorCode::BadAesKey, "readArchives.mountPoint");
        return fail(ErrorCode::InvalidData, "readArchives.mountPoint");
    }
    Result<PakPrimaryIndex> primary = parsePakPrimaryIndex(*primaryBytes, *info);
    if (!primary)
        return std::unexpected(primary.error());
    Result<std::vector<std::byte>> directoryBytes = readPakIndex(file, *info, primary->directoryIndexOffset, primary->directoryIndexSize, input.decryptor);
    if (!directoryBytes)
        return std::unexpected(directoryBytes.error());

    MountedArchive archive;
    archive.storage = std::make_unique<Arena>();
    archive.name = archive.storage->copy(file.getName());
    archive.kind = ArchiveKind::Pak;
    archive.priority = getReadOrder(archive.name);
    std::string_view mountPoint = StringView(primary->mountPoint, primary->mountPointWide).toUtf8(*archive.storage);
    archive.tree.mountPoint = normalizeMountPoint(archive.storage->copy(mountPoint));

    PakEntries entries;
    entries.entries.reserve(std::min<std::size_t>(primary->entryCount, directoryBytes->size() / 8));
    Result<void> tree = parsePakDirectoryIndex(*directoryBytes, *info, *primary, *archive.storage, archive.tree, entries);
    if (!tree)
        return std::unexpected(tree.error());
    archive.pak = std::make_unique<PakFile>(*info, std::move(entries), std::move(source), std::move(decompressors), input.decryptor);
    return archive;
}

}

Result<ArchiveSet> readArchives(std::vector<std::unique_ptr<FileSource>> sources, std::unique_ptr<Decryptor> decryptor, std::unique_ptr<Codecs> codecs,
                                std::stop_token stop) {
    ArchiveSet set;
    set.decryptor = std::move(decryptor);
    set.codecs = codecs != nullptr ? std::move(codecs) : std::make_unique<Codecs>();
    set.problemNames = std::make_unique<Arena>();

    auto addProblem = [&](std::string_view name, Error error) { set.problems.push_back(ArchiveProblem{set.problemNames->copy(name), error}); };

    SourceIndex index;
    std::vector<std::size_t> roots;
    for (std::size_t i = 0; i < sources.size(); i++) {
        std::string_view name = sources[i]->getName();
        if (index.contains(name)) {
            addProblem(name, Error{ErrorCode::InvalidData, "readArchives.duplicateName", 0});
            sources[i] = nullptr;
            continue;
        }
        index.add(name, i);
        if (endsWithIgnoreCase(name, ".utoc") || endsWithIgnoreCase(name, ".pak"))
            roots.push_back(i);
    }

    std::vector<std::unique_ptr<FileSource>> rootFiles(roots.size());
    for (std::size_t i = 0; i < roots.size(); i++)
        rootFiles[i] = std::move(sources[roots[i]]);
    std::vector<std::optional<Result<MountedArchive>>> results(roots.size());
    MountInput input{sources, index, set.decryptor.get(), *set.codecs};
    tbb::parallel_for(std::size_t(0), roots.size(), [&](std::size_t i) {
        if (stop.stop_requested()) {
            results[i] = fail(ErrorCode::Cancelled, "readArchives");
            return;
        }
        if (endsWithIgnoreCase(rootFiles[i]->getName(), ".pak"))
            results[i] = mountPak(rootFiles[i], input);
        else
            results[i] = mountIoStore(*rootFiles[i], input);
    });
    if (stop.stop_requested())
        return fail(ErrorCode::Cancelled, "readArchives");

    for (std::size_t i = 0; i < results.size(); i++) {
        Result<MountedArchive>& result = *results[i];
        if (!result) {
            addProblem(rootFiles[i]->getName(), result.error());
            continue;
        }
        std::uint32_t archiveIndex = static_cast<std::uint32_t>(set.archives.size());
        for (FileEntry& file : result->tree.files)
            file.archive = archiveIndex;
        set.archives.push_back(std::move(*result));
    }
    return set;
}

std::uint64_t hashPath(std::string_view path) {
    std::uint64_t hash = 0;
    std::size_t start = 0;
    while (start <= path.size()) {
        std::size_t end = path.find('/', start);
        if (end == std::string_view::npos)
            end = path.size();
        if (end > start)
            hash = combinePath(hash, hashName(path.substr(start, end - start)));
        start = end + 1;
    }
    return hash;
}

namespace {

constexpr std::size_t shardCount = 256;
std::size_t getShard(std::uint64_t hash) {
    return static_cast<std::size_t>((hash >> 32) & (shardCount - 1));
}

struct ChunkIdHash {
    using is_avalanching = void;
    std::uint64_t operator()(const FIoChunkId& id) const noexcept {
        std::string_view bytes{reinterpret_cast<const char*>(id.bytes), sizeof(id.bytes)};
        return ankerl::unordered_dense::hash<std::string_view>{}(bytes);
    }
};

struct PathHash {
    using is_avalanching = void;
    std::uint64_t operator()(std::uint64_t hash) const noexcept { return hash; }
};

struct Candidate {
    std::uint32_t archive;
    std::uint32_t index;
};

struct ShardedCandidates {
    std::vector<Candidate> candidates;
    std::vector<std::size_t> shardStart;

    std::span<const Candidate> getShard(std::size_t shard) const {
        return std::span(candidates).subspan(shardStart[shard], shardStart[shard + 1] - shardStart[shard]);
    }
};

ShardedCandidates scatterByShard(std::span<const std::vector<std::uint64_t>> hashes) {
    std::size_t archiveCount = hashes.size();
    std::vector<std::size_t> cursors(archiveCount * shardCount, 0);
    tbb::parallel_for(std::size_t(0), archiveCount, [&](std::size_t archive) {
        std::size_t* counts = cursors.data() + archive * shardCount;
        for (std::uint64_t hash : hashes[archive])
            counts[getShard(hash)]++;
    });

    ShardedCandidates result;
    result.shardStart.resize(shardCount + 1);
    std::size_t total = 0;
    for (std::size_t shard = 0; shard < shardCount; shard++) {
        result.shardStart[shard] = total;
        for (std::size_t archive = 0; archive < archiveCount; archive++) {
            std::size_t count = cursors[archive * shardCount + shard];
            cursors[archive * shardCount + shard] = total;
            total += count;
        }
    }
    result.shardStart[shardCount] = total;

    result.candidates.resize(total);
    tbb::parallel_for(std::size_t(0), archiveCount, [&](std::size_t archive) {
        std::size_t* cursor = cursors.data() + archive * shardCount;
        const std::vector<std::uint64_t>& archiveHashes = hashes[archive];
        for (std::size_t i = 0; i < archiveHashes.size(); i++)
            result.candidates[cursor[getShard(archiveHashes[i])]++] = Candidate{static_cast<std::uint32_t>(archive), static_cast<std::uint32_t>(i)};
    });
    return result;
}

}

struct FileSystem::Tables {
    std::vector<ankerl::unordered_dense::map<FIoChunkId, ChunkLocation, ChunkIdHash>> chunks;
    std::vector<ankerl::unordered_dense::map<std::uint64_t, std::uint32_t, PathHash>> paths;
};

FileSystem::FileSystem(ArchiveSet archives) : set(std::move(archives)), tables(std::make_unique<Tables>()) {
    std::size_t archiveCount = set.archives.size();
    UE5PARSE_CHECK(archiveCount < none);

    auto isBetter = [&](std::uint32_t archive, std::uint32_t other) {
        const MountedArchive& a = set.archives[archive];
        const MountedArchive& b = set.archives[other];
        if (a.priority != b.priority)
            return a.priority > b.priority;
        std::uint64_t aId = a.ioStore != nullptr ? a.ioStore->getToc().getContainerId() : 0;
        std::uint64_t bId = b.ioStore != nullptr ? b.ioStore->getToc().getContainerId() : 0;
        if (aId != bId)
            return aId > bId;
        return a.name > b.name;
    };

    std::vector<std::vector<std::uint64_t>> hashes(archiveCount);
    tbb::parallel_for(std::size_t(0), archiveCount, [&](std::size_t archive) {
        const IoStoreContainer* container = set.archives[archive].ioStore.get();
        if (container == nullptr)
            return;
        const IoStoreToc& toc = container->getToc();
        hashes[archive].resize(toc.getChunkCount());
        for (std::size_t i = 0; i < toc.getChunkCount(); i++)
            hashes[archive][i] = ChunkIdHash{}(toc.getChunkId(i));
    });
    ShardedCandidates chunks = scatterByShard(hashes);
    tables->chunks.resize(shardCount);
    tbb::parallel_for(std::size_t(0), shardCount, [&](std::size_t shard) {
        std::span<const Candidate> candidates = chunks.getShard(shard);
        auto& map = tables->chunks[shard];
        map.reserve(candidates.size());
        for (const Candidate& candidate : candidates) {
            const FIoChunkId& id = set.archives[candidate.archive].ioStore->getToc().getChunkId(candidate.index);
            auto [found, inserted] = map.try_emplace(id, ChunkLocation{candidate.archive, candidate.index});
            if (!inserted && isBetter(candidate.archive, found->second.archive))
                found->second = ChunkLocation{candidate.archive, candidate.index};
        }
    });

    for (std::size_t archive = 0; archive < archiveCount; archive++)
        hashes[archive] = set.archives[archive].tree.pathHashes;
    ShardedCandidates paths = scatterByShard(hashes);
    hashes = {};
    std::vector<ankerl::unordered_dense::map<std::uint64_t, Candidate, PathHash>> winners(shardCount);
    tbb::parallel_for(std::size_t(0), shardCount, [&](std::size_t shard) {
        std::span<const Candidate> candidates = paths.getShard(shard);
        auto& map = winners[shard];
        map.reserve(candidates.size());
        for (const Candidate& candidate : candidates) {
            std::uint64_t hash = set.archives[candidate.archive].tree.pathHashes[candidate.index];
            auto [found, inserted] = map.try_emplace(hash, candidate);
            if (!inserted && isBetter(candidate.archive, found->second.archive))
                found->second = candidate;
        }
    });

    std::vector<std::size_t> fileStart(shardCount + 1, 0);
    for (std::size_t shard = 0; shard < shardCount; shard++)
        fileStart[shard + 1] = fileStart[shard] + winners[shard].size();
    files.resize(fileStart[shardCount]);
    tables->paths.resize(shardCount);
    tbb::parallel_for(std::size_t(0), shardCount, [&](std::size_t shard) {
        auto& map = tables->paths[shard];
        map.reserve(winners[shard].size());
        std::size_t next = fileStart[shard];
        for (const auto& [hash, candidate] : winners[shard]) {
            files[next] = set.archives[candidate.archive].tree.files[candidate.index];
            map.emplace(hash, static_cast<std::uint32_t>(next));
            next++;
        }
    });
    for (MountedArchive& archive : set.archives) {
        archive.tree.files = {};
        archive.tree.pathHashes = {};
    }
}

FileSystem::FileSystem(FileSystem&&) noexcept = default;
FileSystem& FileSystem::operator=(FileSystem&&) noexcept = default;
FileSystem::~FileSystem() = default;

std::optional<FileEntry> FileSystem::findFile(std::string_view path) const {
    std::uint64_t hash = hashPath(path);
    const auto& map = tables->paths[getShard(hash)];
    auto found = map.find(hash);
    if (found == map.end())
        return std::nullopt;
    return files[found->second];
}

std::string_view FileSystem::writePath(const FileEntry& file, std::span<char> buffer) const {
    const DirectoryTree& tree = set.archives[file.archive].tree;
    std::string_view fileName = tree.getFileName(file.name);
    std::size_t size = tree.mountPoint.size() + fileName.size();
    for (std::uint32_t directory = file.directory; directory != none; directory = tree.parents[directory])
        if (!tree.names[directory].empty())
            size += tree.names[directory].size() + 1;
    if (size > buffer.size())
        return {};

    std::size_t end = size;
    end -= fileName.size();
    std::memcpy(buffer.data() + end, fileName.data(), fileName.size());
    for (std::uint32_t directory = file.directory; directory != none; directory = tree.parents[directory]) {
        std::string_view name = tree.names[directory];
        if (name.empty())
            continue;
        buffer[--end] = '/';
        end -= name.size();
        std::memcpy(buffer.data() + end, name.data(), name.size());
    }
    UE5PARSE_CHECK(end == tree.mountPoint.size());
    std::memcpy(buffer.data(), tree.mountPoint.data(), tree.mountPoint.size());
    return {buffer.data(), size};
}

std::uint64_t FileSystem::getFileSize(const FileEntry& file) const {
    const MountedArchive& archive = set.archives[file.archive];
    if (archive.pak != nullptr)
        return archive.pak->getFileSize(file.chunk);
    return archive.ioStore->getChunkSize(file.chunk);
}

Result<void> FileSystem::readFile(const FileEntry& file, std::uint64_t offset, std::span<std::byte> output, std::stop_token stop) const {
    const MountedArchive& archive = set.archives[file.archive];
    if (archive.pak != nullptr)
        return archive.pak->read(file.chunk, offset, output, stop);
    return archive.ioStore->read(file.chunk, offset, output, stop);
}

std::optional<ChunkLocation> FileSystem::findChunk(const FIoChunkId& id) const {
    const auto& map = tables->chunks[getShard(ChunkIdHash{}(id))];
    auto found = map.find(id);
    if (found == map.end())
        return std::nullopt;
    return found->second;
}

std::uint64_t FileSystem::getChunkSize(ChunkLocation location) const {
    return set.archives[location.archive].ioStore->getChunkSize(location.chunk);
}

Result<void> FileSystem::readChunk(ChunkLocation location, std::uint64_t offset, std::span<std::byte> output, std::stop_token stop) const {
    return set.archives[location.archive].ioStore->read(location.chunk, offset, output, stop);
}

Result<std::vector<std::byte>> FileSystem::readChunk(const FIoChunkId& id, std::stop_token stop) const {
    std::optional<ChunkLocation> location = findChunk(id);
    if (!location)
        return fail(ErrorCode::MissingChunk, "FileSystem.readChunk");
    std::vector<std::byte> bytes(getChunkSize(*location));
    Result<void> read = readChunk(*location, 0, bytes, stop);
    if (!read)
        return std::unexpected(read.error());
    return bytes;
}

}
