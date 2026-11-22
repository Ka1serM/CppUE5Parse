#include "CppUE5Parse/VFS/FileSystem.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <climits>
#include <cstdint>
#include <cstring>
#include <string>

#include <ankerl/unordered_dense.h>
#include <city.h>
#include <tbb/parallel_for.h>

#include "CppUE5Parse/Core/ByteReader.h"
#include "CppUE5Parse/Core/Check.h"
#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Text.h"
#include "CppUE5Parse/GameTypes/DragonSword/DragonSwordIndex.h"

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

class DirectoryBuilder {
public:
    DirectoryBuilder(DirectoryTree& tree, Arena& storage) : tree(tree), storage(storage) {
        tree.parents.assign(1, none);
        tree.names.assign(1, {});
        hashes.push_back(hashPath(tree.mountPoint));
        byHash.emplace(hashes[0], 0);
    }

    std::uint32_t add(std::string_view path) {
        std::uint32_t directory = 0;
        for (std::size_t start = 0; start < path.size();) {
            std::size_t end = path.find('/', start);
            if (end == std::string_view::npos)
                end = path.size();
            if (end > start)
                directory = addChild(directory, path.substr(start, end - start));
            start = end + 1;
        }
        return directory;
    }

    Result<void> addFile(std::uint32_t directory, std::string_view name, std::uint32_t chunk) {
        if (name.empty())
            return fail(ErrorCode::InvalidData, "DirectoryBuilder.fileName");
        Result<std::uint32_t> kept = addFileName(tree, name);
        if (!kept)
            return std::unexpected(kept.error());
        tree.files.push_back(FileEntry{0, directory, chunk, *kept});
        tree.pathHashes.push_back(combinePath(hashes[directory], hashName(name)));
        return {};
    }

private:
    std::uint32_t addChild(std::uint32_t parent, std::string_view component) {
        std::uint64_t hash = combinePath(hashes[parent], hashName(component));
        auto [found, inserted] = byHash.try_emplace(hash, static_cast<std::uint32_t>(tree.names.size()));
        if (inserted) {
            tree.parents.push_back(parent);
            tree.names.push_back(storage.copy(component));
            hashes.push_back(hash);
        }
        return found->second;
    }

    DirectoryTree& tree;
    Arena& storage;
    std::vector<std::uint64_t> hashes;
    ankerl::unordered_dense::map<std::uint64_t, std::uint32_t> byHash;
};

//! Need for Speed Mobile keeps the part of each full path after the first "../../../", as CUE4Parse does.
Result<void> parseNeedForSpeedMobileDirectoryIndex(std::string_view mountPoint, std::span<const FIoDirectoryIndexEntry> directories,
                                                   std::span<const FIoFileIndexEntry> files, std::span<const std::string_view> strings,
                                                   std::size_t chunkCount, Arena& storage, DirectoryTree& tree) {
    constexpr std::string_view relativeRoot = "../../../";
    tree.mountPoint = {};
    DirectoryBuilder builder(tree, storage);
    std::vector<std::uint8_t> visitedDirectories(directories.size(), 0);
    std::vector<std::uint8_t> visitedFiles(files.size(), 0);
    auto getString = [&](std::uint32_t name, std::string_view& text) {
        text = {};
        if (name == none)
            return true;
        if (name >= strings.size())
            return false;
        text = strings[name];
        return true;
    };
    std::vector<std::pair<std::uint32_t, std::string>> stack{{0, std::string(mountPoint)}};
    visitedDirectories[0] = 1;
    while (!stack.empty()) {
        auto [directory, parentPath] = std::move(stack.back());
        stack.pop_back();
        const FIoDirectoryIndexEntry& entry = directories[directory];
        std::string_view directoryName;
        if (!getString(entry.name, directoryName))
            return fail(ErrorCode::InvalidData, "parseDirectoryIndex.name");
        std::string path = parentPath;
        if (!directoryName.empty())
            path.append(directoryName).push_back('/');
        for (std::uint32_t file = entry.firstFile; file != none; file = files[file].nextFile) {
            if (file >= files.size() || visitedFiles[file])
                return fail(ErrorCode::InvalidData, "parseDirectoryIndex.file");
            visitedFiles[file] = 1;
            std::string_view name;
            if (!getString(files[file].name, name) || name.empty())
                return fail(ErrorCode::InvalidData, "parseDirectoryIndex.fileName");
            if (files[file].userData >= chunkCount)
                return fail(ErrorCode::InvalidData, "parseDirectoryIndex.chunk");
            std::string fullPath = path + std::string(name);
            std::string_view kept = fullPath;
            std::size_t found = kept.find(relativeRoot);
            if (found != std::string_view::npos)
                kept.remove_prefix(found + relativeRoot.size());
            std::size_t slash = kept.rfind('/');
            std::uint32_t parent = slash == std::string_view::npos ? 0 : builder.add(kept.substr(0, slash));
            Result<void> added = builder.addFile(parent, slash == std::string_view::npos ? kept : kept.substr(slash + 1), files[file].userData);
            if (!added)
                return added;
        }
        for (std::uint32_t child = entry.firstChild; child != none; child = directories[child].nextSibling) {
            if (child >= directories.size() || visitedDirectories[child])
                return fail(ErrorCode::InvalidData, "parseDirectoryIndex.directory");
            visitedDirectories[child] = 1;
            stack.emplace_back(child, path);
        }
    }
    tree.fileNames.shrink_to_fit();
    return {};
}

Result<void> parseDirectoryIndex(std::span<const std::byte> stored, std::size_t chunkCount, EGame game, Arena& storage, DirectoryTree& tree) {
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
    if (game == EGame::GAME_NeedForSpeedMobile)
        return parseNeedForSpeedMobileDirectoryIndex(tree.mountPoint, directories, files, strings, chunkCount, storage, tree);

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
    EGame game;
};

Result<MountedArchive> mountIoStore(const FileSource& tocFile, MountInput& input) {
    Result<IoStoreToc> toc = readToc(tocFile, input.game);
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
    Result<void> tree = parseDirectoryIndex(*index, parsed.getChunkCount(), input.game, *archive.storage, archive.tree);
    if (!tree)
        return std::unexpected(tree.error());
    return archive;
}

std::string_view readUtf8Name(ByteReader& reader) {
    std::span<const std::byte> bytes = reader.readBytes(reader.readCount(1));
    return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

Result<std::uint32_t> copyNonEncodedEntry(const PakPrimaryIndex& primary, std::int32_t location, PakEntries& entries) {
    std::size_t nonEncoded = static_cast<std::size_t>(-(static_cast<std::int64_t>(location) + 1));
    if (nonEncoded >= primary.nonEncoded.entries.size())
        return fail(ErrorCode::InvalidData, "parsePakDirectoryIndex.location");
    FPakEntry copy = primary.nonEncoded.entries[nonEncoded];
    std::span<const FPakCompressedBlock> blocks = std::span(primary.nonEncoded.blocks).subspan(copy.firstBlock, copy.blockCount);
    copy.firstBlock = static_cast<std::uint32_t>(entries.blocks.size());
    entries.blocks.insert(entries.blocks.end(), blocks.begin(), blocks.end());
    entries.entries.push_back(copy);
    return static_cast<std::uint32_t>(entries.entries.size() - 1);
}

//! Crystal of Atlan leaves files out of the directory index and lists them in a path hash map after it; CUE4Parse names the files that it cannot recover by their hash.
Result<void> addCrystalOfAtlanHashedFiles(ByteReader& reader, const FPakInfo& info, const PakPrimaryIndex& primary,
                                          const ankerl::unordered_dense::set<std::int32_t>& usedLocations, Arena& storage, DirectoryBuilder& builder,
                                          DirectoryTree& tree, PakEntries& entries) {
    std::uint32_t count = reader.readCount(12);
    std::uint32_t directory = tree.mountPoint.empty() ? builder.add("Seria/Content") : 0;
    std::array<char, 24> name{};
    for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
        std::uint64_t hash = reader.read<std::uint64_t>();
        std::int32_t location = reader.read<std::int32_t>();
        if (reader.hasFailed())
            break;
        if (location == INT32_MIN || usedLocations.contains(location))
            continue;
        Result<std::uint32_t> entry = location >= 0 ? decodePakEntry(primary.encodedEntries, static_cast<std::size_t>(location), info, entries)
                                                    : copyNonEncodedEntry(primary, location, entries);
        if (!entry)
            return std::unexpected(entry.error());
        auto written = std::to_chars(name.data(), name.data() + name.size(), hash);
        Result<void> added = builder.addFile(directory, storage.copy(std::string_view(name.data(), written.ptr)), *entry);
        if (!added)
            return added;
    }
    return reader.getResult();
}

Result<void> parsePakDirectoryIndex(std::span<const std::byte> index, const FPakInfo& info, const PakPrimaryIndex& primary, Arena& storage, DirectoryTree& tree,
                                    PakEntries& entries) {
    ByteReader reader{index};
    //! Crystal of Atlan and Dragon Sword store file names as FString in every version.
    bool utf8Names = info.version >= EPakFileVersion::Utf8PakDirectory && info.game != EGame::GAME_CrystalOfAtlan
                     && info.game != EGame::GAME_DragonSwordAwakening;
    bool partial = needsEntryPath(info.game);
    DirectoryBuilder builder(tree, storage);
    ankerl::unordered_dense::set<std::int32_t> usedLocations;
    std::uint64_t fileTotal = 0;
    std::uint32_t directoryCount = reader.readCount(8);
    for (std::uint32_t i = 0; i < directoryCount && !reader.hasFailed(); i++) {
        std::string_view directoryName = readUtf8String(reader, storage);
        std::uint32_t directory = builder.add(directoryName);
        if (!tree.mountPoint.empty() && tree.mountPoint.back() == '/' && directoryName.starts_with('/'))
            directoryName.remove_prefix(1);
        std::uint32_t fileCount = reader.readCount(8);
        fileTotal += fileCount;
        for (std::uint32_t j = 0; j < fileCount && !reader.hasFailed(); j++) {
            std::string_view name = utf8Names ? readUtf8Name(reader) : readUtf8String(reader, storage);
            std::int32_t location = reader.read<std::int32_t>();
            if (reader.hasFailed())
                break;
            if (location == INT32_MIN)
                continue;
            if (info.game == EGame::GAME_CrystalOfAtlan)
                usedLocations.insert(location);
            Result<std::uint32_t> entry = location >= 0 ? decodePakEntry(primary.encodedEntries, static_cast<std::size_t>(location), info, entries)
                                                        : copyNonEncodedEntry(primary, location, entries);
            if (!entry)
                return std::unexpected(entry.error());
            if (partial)
                applyEntryPath(info, std::string(tree.mountPoint).append(directoryName).append(name), entries.entries[*entry]);
            Result<void> added = builder.addFile(directory, name, *entry);
            if (!added)
                return added;
        }
    }
    if (info.game == EGame::GAME_CrystalOfAtlan && !reader.hasFailed() && fileTotal < primary.entryCount) {
        Result<void> named = addCrystalOfAtlanHashedFiles(reader, info, primary, usedLocations, storage, builder, tree, entries);
        if (!named)
            return named;
    }
    tree.fileNames.shrink_to_fit();
    return reader.getResult();
}

Result<void> addLegacyFile(DirectoryBuilder& builder, std::string_view path, std::uint32_t entry) {
    std::size_t slash = path.rfind('/');
    std::uint32_t directory = slash == std::string_view::npos ? 0 : builder.add(path.substr(0, slash));
    return builder.addFile(directory, slash == std::string_view::npos ? path : path.substr(slash + 1), entry);
}

//! Dragon Quest XI XORs the UTF-16 file names and renames the Game folder to JackGame.
Result<void> parseDragonQuestXIIndex(ByteReader& reader, const FPakInfo& info, Arena& storage, DirectoryTree& tree, PakEntries& entries) {
    constexpr std::uint8_t xorKey[] = {0xDE, 0x00, 0xAD, 0x00, 0xFA, 0x00, 0xDE, 0x00, 0xBE, 0x00, 0xEF, 0x00, 0xCA, 0x00, 0xFE, 0x00};
    if (tree.mountPoint.starts_with("Game"))
        tree.mountPoint = storage.copy(std::string("Jack").append(tree.mountPoint));
    std::uint32_t count = reader.readCount(8);
    entries.entries.reserve(count);
    DirectoryBuilder builder(tree, storage);
    std::vector<std::byte> name;
    for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
        std::int64_t length = -2 * static_cast<std::int64_t>(reader.read<std::int32_t>());
        if (length < 2 || static_cast<std::uint64_t>(length) > reader.getRemaining())
            return fail(ErrorCode::InvalidData, "parseDragonQuestXIIndex.name");
        std::span<const std::byte> stored = reader.readBytes(static_cast<std::size_t>(length));
        name.assign(stored.begin(), stored.end() - 2);
        for (std::size_t j = 0; j < name.size(); j++)
            name[j] ^= static_cast<std::byte>(xorKey[j & 0xF]);
        std::string_view path = utf16ToUtf8(name, storage);
        if (tree.mountPoint.empty() && path.starts_with("Game"))
            path = storage.copy(std::string("Jack").append(path));
        Result<std::uint32_t> entry = readPakEntry(reader, info, entries);
        if (!entry)
            return std::unexpected(entry.error());
        Result<void> added = addLegacyFile(builder, path, *entry);
        if (!added)
            return added;
    }
    tree.fileNames.shrink_to_fit();
    return reader.getResult();
}

//! Conan Exiles stores unused blocks for gtp and gts files, which are stored uncompressed.
void fixConanExilesEntry(std::string_view path, PakEntries& entries, std::uint32_t entry) {
    FPakEntry& stored = entries.entries[entry];
    if (stored.compressionMethodIndex == 0 || !(endsWithIgnoreCase(path, "gtp") || endsWithIgnoreCase(path, "gts")))
        return;
    stored.compressionMethodIndex = 0;
    stored.compressionBlockSize = 0;
}

Result<void> parsePakLegacyIndex(ByteReader& reader, const FPakInfo& info, Arena& storage, DirectoryTree& tree, PakEntries& entries) {
    if (info.game == EGame::GAME_DragonQuestXI)
        return parseDragonQuestXIIndex(reader, info, storage, tree, entries);
    std::uint32_t count = reader.readCount(8);
    //! Transformers Online stores the file count plus 100.
    if (info.game == EGame::GAME_TransformersOnline) {
        if (count < 100)
            return fail(ErrorCode::InvalidData, "parsePakLegacyIndex.count");
        count -= 100;
    }
    entries.entries.reserve(count);
    DirectoryBuilder builder(tree, storage);
    for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
        std::string_view path = readUtf8String(reader, storage);
        Result<std::uint32_t> entry = readPakEntry(reader, info, entries);
        if (!entry)
            return std::unexpected(entry.error());
        if (info.game == EGame::GAME_ConanExiles)
            fixConanExilesEntry(path, entries, *entry);
        if (needsEntryPath(info.game))
            applyEntryPath(info, std::string(tree.mountPoint).append(path), entries.entries[*entry]);
        std::size_t slash = path.rfind('/');
        std::uint32_t directory = slash == std::string_view::npos ? 0 : builder.add(path.substr(0, slash));
        Result<void> added = builder.addFile(directory, slash == std::string_view::npos ? path : path.substr(slash + 1), *entry);
        if (!added)
            return added;
    }
    tree.fileNames.shrink_to_fit();
    return reader.getResult();
}

//! Delta Force XORs its pak data with a byte that its primary index starts with four times; Dead by Daylight XORs its index with footer data.
Result<void> unmaskPakIndex(FPakInfo& info, std::span<std::byte> bytes) {
    if ((info.game == EGame::GAME_DeadByDaylight || info.game == EGame::GAME_DeadByDaylight_Old) && info.customEncryptionDataSize == 28)
        for (std::size_t i = 0; i < bytes.size(); i++)
            bytes[i] ^= info.customEncryptionData[i % 28];
    if (info.game != EGame::GAME_DeltaForce)
        return {};
    if (info.customEncryptionDataSize == 0) {
        if (bytes.size() < 4 || bytes[1] != bytes[0] || bytes[2] != bytes[0] || bytes[3] != bytes[0])
            return fail(ErrorCode::BadAesKey, "readPakIndex.deltaForceMask");
        std::fill_n(info.customEncryptionData.begin(), 8, bytes[0]);
        info.customEncryptionDataSize = 8;
    }
    for (std::size_t i = 0; i < bytes.size() / 8 * 8; i++)
        bytes[i] ^= info.customEncryptionData[i % 8];
    return {};
}

Result<std::vector<std::byte>> readPakIndex(const FileSource& file, FPakInfo& info, std::uint64_t offset, std::uint64_t size, const Decryptor* decryptor,
                                            EncryptedPart part) {
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
    request.part = part;
    request.offset = offset;
    request.game = info.game;
    request.archiveName = file.getName();
    Result<void> done = decryptor->decrypt(request);
    if (!done)
        return std::unexpected(done.error());
    Result<void> unmasked = unmaskPakIndex(info, bytes);
    if (!unmasked)
        return std::unexpected(unmasked.error());
    return bytes;
}

void setMountPoint(MountedArchive& archive, std::span<const std::byte> mountPoint, bool wide) {
    std::string_view text = StringView(mountPoint, wide).toUtf8(*archive.storage);
    archive.tree.mountPoint = normalizeMountPoint(archive.storage->copy(text));
}

//! Follows the checks of CUE4Parse's FArchive.ReadFString, which Game for Peace uses to tell its two index layouts apart.
bool isReadableFString(ByteReader reader) {
    std::int32_t length = reader.read<std::int32_t>();
    if (reader.hasFailed() || length == INT32_MIN)
        return false;
    std::size_t units = static_cast<std::size_t>(length < 0 ? -static_cast<std::int64_t>(length) : length);
    if (units > reader.getRemaining())
        return false;
    if (units == 0)
        return true;
    std::span<const std::byte> bytes = reader.readBytes(length < 0 ? units * 2 : units);
    if (reader.hasFailed())
        return false;
    return bytes.back() == std::byte{0} && (length > 0 || bytes[bytes.size() - 2] == std::byte{0});
}

//! Reads a count that Game for Peace stores as int64 and checks it against the bytes that each element needs at least.
std::uint32_t readGameForPeaceCount(ByteReader& reader, std::size_t elementSize) {
    std::int64_t count = reader.read<std::int64_t>();
    if (count < 0 || static_cast<std::uint64_t>(count) > reader.getRemaining() / elementSize) {
        reader.fail(ErrorCode::InvalidData, "parseGameForPeaceIndex.count");
        return 0;
    }
    return static_cast<std::uint32_t>(count);
}

//! Older Game for Peace paks store the entries first and a directory index after the index, which starts with its own size.
Result<void> parseGameForPeaceIndex(ByteReader& reader, const FileSource& file, FPakInfo& info, const Decryptor* decryptor, Arena& storage,
                                    DirectoryTree& tree, PakEntries& entries) {
    std::uint32_t count = reader.readCount(8);
    entries.entries.reserve(count);
    for (std::uint32_t i = 0; i < count && !reader.hasFailed(); i++) {
        Result<std::uint32_t> entry = readPakEntry(reader, info, entries);
        if (!entry)
            return std::unexpected(entry.error());
    }
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    std::uint64_t sizeOffset = info.indexOffset + info.indexSize;
    std::int64_t directorySize = 0;
    if (sizeOffset > file.getSize() || file.getSize() - sizeOffset < sizeof(directorySize))
        return fail(ErrorCode::InvalidData, "parseGameForPeaceIndex.size", sizeOffset);
    Result<void> readSize = file.read(sizeOffset, std::as_writable_bytes(std::span(&directorySize, 1)));
    if (!readSize)
        return readSize;
    if (directorySize < 0)
        return fail(ErrorCode::InvalidData, "parseGameForPeaceIndex.size", sizeOffset);
    Result<std::vector<std::byte>> directoryBytes = readPakIndex(file, info, sizeOffset + sizeof(directorySize), static_cast<std::uint64_t>(directorySize),
                                                                 decryptor, EncryptedPart::PakDirectoryIndex);
    if (!directoryBytes)
        return std::unexpected(directoryBytes.error());
    ByteReader directories{*directoryBytes};
    DirectoryBuilder builder(tree, storage);
    std::uint32_t directoryCount = readGameForPeaceCount(directories, 12);
    for (std::uint32_t i = 0; i < directoryCount && !directories.hasFailed(); i++) {
        std::string_view directoryName = readUtf8String(directories, storage);
        std::uint32_t directory = builder.add(directoryName);
        if (!tree.mountPoint.empty() && tree.mountPoint.back() == '/' && directoryName.starts_with('/'))
            directoryName.remove_prefix(1);
        std::uint32_t fileCount = readGameForPeaceCount(directories, 8);
        for (std::uint32_t j = 0; j < fileCount && !directories.hasFailed(); j++) {
            std::string_view name = readUtf8String(directories, storage);
            std::int32_t location = directories.read<std::int32_t>();
            if (directories.hasFailed())
                break;
            if (location < 0 || static_cast<std::uint32_t>(location) >= entries.entries.size())
                return fail(ErrorCode::InvalidData, "parseGameForPeaceIndex.location");
            applyEntryPath(info, std::string(tree.mountPoint).append(directoryName).append(name), entries.entries[static_cast<std::uint32_t>(location)]);
            Result<void> added = builder.addFile(directory, name, static_cast<std::uint32_t>(location));
            if (!added)
                return added;
        }
    }
    tree.fileNames.shrink_to_fit();
    return directories.getResult();
}

Result<void> readLegacyTree(const FileSource& file, std::span<const std::byte> index, FPakInfo& info, const MountInput& input, MountedArchive& archive,
                            PakEntries& entries) {
    ByteReader reader{index};
    bool wide = false;
    std::span<const std::byte> mountPoint = reader.readStringBytes(wide);
    setMountPoint(archive, mountPoint, wide);
    ByteReader probe = reader;
    probe.skip(sizeof(std::int32_t));
    if (info.game == EGame::GAME_GameForPeace && !isReadableFString(probe))
        return parseGameForPeaceIndex(reader, file, info, input.decryptor, *archive.storage, archive.tree, entries);
    return parsePakLegacyIndex(reader, info, *archive.storage, archive.tree, entries);
}

Result<void> readPathHashTree(const FileSource& file, std::span<const std::byte> index, FPakInfo& info, const MountInput& input,
                              MountedArchive& archive, PakEntries& entries) {
    Result<PakPrimaryIndex> primary = parsePakPrimaryIndex(index, info);
    if (!primary)
        return std::unexpected(primary.error());
    Result<std::vector<std::byte>> directoryBytes = readPakIndex(file, info, primary->directoryIndexOffset, primary->directoryIndexSize, input.decryptor,
                                                                       EncryptedPart::PakDirectoryIndex);
    if (!directoryBytes)
        return std::unexpected(directoryBytes.error());
    if (info.game == EGame::GAME_DragonSwordAwakening) {
        Result<void> decoded = decodeDragonSwordDirectoryIndex(*directoryBytes);
        if (!decoded)
            return decoded;
    }
    setMountPoint(archive, primary->mountPoint, primary->mountPointWide);
    entries.entries.reserve(std::min<std::size_t>(primary->entryCount, directoryBytes->size() / 8));
    return parsePakDirectoryIndex(*directoryBytes, info, *primary, *archive.storage, archive.tree, entries);
}

Result<MountedArchive> mountPak(std::unique_ptr<FileSource>& source, MountInput& input) {
    const FileSource& file = *source;
    std::vector<std::byte> publicKeyPem;
    if (input.game == EGame::GAME_InZOI) {
        std::string_view name = file.getName();
        std::size_t slash = name.find_last_of("/\\");
        std::string keyName(slash == std::string_view::npos ? std::string_view{} : name.substr(0, slash + 1));
        keyName += "public_key.pem";
        std::optional<std::size_t> keySource = input.index.find(keyName);
        if (keySource && input.sources[*keySource] != nullptr) {
            const FileSource& keyFile = *input.sources[*keySource];
            publicKeyPem.resize(static_cast<std::size_t>(keyFile.getSize()));
            Result<void> readKey = keyFile.read(0, publicKeyPem);
            if (!readKey)
                return std::unexpected(readKey.error());
        }
    }
    Result<FPakInfo> info = readPakInfo(file, input.game, publicKeyPem);
    if (!info)
        return std::unexpected(info.error());

    std::vector<const Decompressor*> decompressors(info->methodCount, nullptr);
    for (std::size_t i = 1; i < decompressors.size(); i++) {
        std::string_view method = info->getMethodName(i);
        //! Century: Age of Ashes names its XORed LZ4 blocks PWC.
        if (input.game == EGame::GAME_CenturyAgeofAshes && method == "PWC")
            method = "LZ4";
        decompressors[i] = input.codecs.find(FName(method));
        if (decompressors[i] == nullptr)
            return fail(ErrorCode::UnknownCompression, "readArchives.method");
    }
    if (info->encryptedIndex && (input.decryptor == nullptr || (!needsNoKey(input.game) && !input.decryptor->hasKey(info->encryptionKeyGuid))))
        return fail(ErrorCode::MissingKey, "readArchives.key");

    Result<std::vector<std::byte>> primaryBytes = readPakIndex(file, *info, info->indexOffset, info->indexSize, input.decryptor, EncryptedPart::PakIndex);
    if (!primaryBytes)
        return std::unexpected(primaryBytes.error());
    if (input.game == EGame::GAME_DragonSwordAwakening && info->version >= EPakFileVersion::PathHashIndex) {
        Result<void> decoded = decodeDragonSwordPrimaryIndex(*primaryBytes);
        if (!decoded)
            return std::unexpected(decoded.error());
    }
    std::span<const std::byte> primaryView = *primaryBytes;
    if (!hasValidMountPoint(primaryView.subspan(std::min(getPakMountPointOffset(*info), primaryView.size())))) {
        if (info->encryptedIndex)
            return fail(ErrorCode::BadAesKey, "readArchives.mountPoint");
        return fail(ErrorCode::InvalidData, "readArchives.mountPoint");
    }

    MountedArchive archive;
    archive.storage = std::make_unique<Arena>();
    archive.name = archive.storage->copy(file.getName());
    archive.kind = ArchiveKind::Pak;
    archive.priority = getReadOrder(archive.name);
    PakEntries entries;
    Result<void> tree = info->version < EPakFileVersion::PathHashIndex ? readLegacyTree(file, *primaryBytes, *info, input, archive, entries)
                                                                       : readPathHashTree(file, *primaryBytes, *info, input, archive, entries);
    if (!tree)
        return std::unexpected(tree.error());
    archive.pak = std::make_unique<PakFile>(*info, std::move(entries), std::move(source), std::move(decompressors), input.decryptor);
    return archive;
}

}

Result<ArchiveSet> readArchives(std::vector<std::unique_ptr<FileSource>> sources, std::unique_ptr<Decryptor> decryptor, std::unique_ptr<Codecs> codecs,
                                EGame game, std::stop_token stop) {
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
    MountInput input{sources, index, set.decryptor.get(), *set.codecs, game};
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
