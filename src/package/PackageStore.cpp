#include <tbb/parallel_for.h>

#include <ankerl/unordered_dense.h>

#include <cstring>
#include <optional>

#include "ue5parse/package/PackageStore.h"

namespace ue5parse {

namespace {

constexpr std::uint32_t containerHeaderSignature = 0x496f436e;

// EIoContainerHeaderVersion.
enum class EIoContainerHeaderVersion : std::uint32_t {
    Initial = 0,
    LocalizedPackages = 1,
    OptionalSegmentPackages = 2,
    NoExportInfo = 3,
    SoftPackageReferences = 4,
    SoftPackageReferencesOffset = 5,
};
constexpr EIoContainerHeaderVersion latestContainerHeaderVersion = EIoContainerHeaderVersion::SoftPackageReferencesOffset;

struct IdHash {
    using is_avalanching = void;
    // Package ids and object indices are CityHash64 values already.
    std::uint64_t operator()(std::uint64_t value) const noexcept { return value; }
};

}

struct ScriptObjects::Index {
    ankerl::unordered_dense::map<std::uint64_t, std::uint32_t, IdHash> entries;
};

ScriptObjects::ScriptObjects() : storage(std::make_unique<Arena>()), index(std::make_unique<Index>()) {}
ScriptObjects::ScriptObjects(ScriptObjects&&) noexcept = default;
ScriptObjects& ScriptObjects::operator=(ScriptObjects&&) noexcept = default;
ScriptObjects::~ScriptObjects() = default;

const FScriptObjectEntry* ScriptObjects::find(FPackageObjectIndex objectIndex) const {
    if (!objectIndex.isScriptImport()) return nullptr;
    auto found = index->entries.find(objectIndex.value);
    return found != index->entries.end() ? &entries[found->second] : nullptr;
}

FName ScriptObjects::getName(const FScriptObjectEntry& entry) const {
    return makeName(names, entry.objectName.getIndex(), entry.objectName.number);
}

Result<ScriptObjects> parseScriptObjects(std::span<const std::byte> chunk) {
    ScriptObjects objects;
    objects.data.assign(chunk.begin(), chunk.end());
    ByteReader reader{objects.data};
    Result<std::vector<FName>> names = readNameBatch(reader, *objects.storage);
    if (!names) return std::unexpected(names.error());
    objects.names = std::move(*names);
    std::uint32_t count = reader.readCount(sizeof(FScriptObjectEntry));
    objects.entries.resize(count);
    reader.readInto(std::span(objects.entries));
    if (reader.hasFailed()) return std::unexpected(reader.getError());
    objects.index->entries.reserve(count);
    for (std::uint32_t i = 0; i < count; i++) objects.index->entries.try_emplace(objects.entries[i].globalIndex.value, i);
    return objects;
}

Result<ScriptObjects> readScriptObjects(const FileSystem& files, std::stop_token stop) {
    Result<std::vector<std::byte>> chunk = files.readChunk(FIoChunkId::create(0, 0, EIoChunkType::ScriptObjects), stop);
    if (!chunk) return std::unexpected(chunk.error());
    return parseScriptObjects(*chunk);
}

struct PackageStore::Container {
    std::vector<std::byte> data;   // the header chunk
    std::vector<FPackageId> imports;   // the imported package ids of all entries
    std::vector<PackageStoreEntry> entries;   // moved to PackageStore::entries at the end of readPackageStore
    ankerl::unordered_dense::map<std::uint64_t, std::uint32_t, IdHash> byId;   // package id -> index in PackageStore::entries
};

PackageStore::PackageStore() = default;
PackageStore::PackageStore(PackageStore&&) noexcept = default;
PackageStore& PackageStore::operator=(PackageStore&&) noexcept = default;
PackageStore::~PackageStore() = default;

std::optional<std::uint32_t> PackageStore::findIndex(std::uint32_t archive, FPackageId id) const {
    if (archive >= containers.size() || containers[archive] == nullptr) return std::nullopt;
    const Container& container = *containers[archive];
    auto found = container.byId.find(id.value);
    if (found == container.byId.end()) return std::nullopt;
    return found->second;
}

const PackageStoreEntry* PackageStore::find(std::uint32_t archive, FPackageId id) const {
    std::optional<std::uint32_t> index = findIndex(archive, id);
    return index ? &entries[*index] : nullptr;
}

namespace {

// The package ids and the store entries of one segment (main or optional). Each entry is a memory image: the imported
// package ids are an FFilePackageStoreEntry CArrayView, a count and an offset from the field itself. The ids are
// copied out, because the blob is not 8-aligned in every version.
Result<void> readStoreEntries(ByteReader& reader, EIoContainerHeaderVersion version, std::vector<PackageStoreEntry>& entries,
                              std::vector<std::pair<std::size_t, std::size_t>>& importRanges, std::vector<FPackageId>& imports,
                              ankerl::unordered_dense::map<std::uint64_t, std::uint32_t, IdHash>& byId) {
    std::uint32_t idCount = reader.readCount(sizeof(FPackageId));
    std::span<const std::byte> ids = reader.readBytes(std::size_t(idCount) * sizeof(FPackageId));
    std::uint32_t blobSize = reader.readCount(1);
    std::span<const std::byte> blob = reader.readBytes(blobSize);
    if (reader.hasFailed()) return std::unexpected(reader.getError());

    std::size_t entrySize = version < EIoContainerHeaderVersion::NoExportInfo ? 24 : 16;
    if (std::size_t(idCount) * entrySize > blob.size()) return fail(ErrorCode::InvalidData, "readPackageStore.entries");
    std::size_t first = entries.size();
    entries.resize(first + idCount);
    importRanges.resize(first + idCount);
    byId.reserve(byId.size() + idCount);
    for (std::uint32_t i = 0; i < idCount; i++) {
        std::size_t at = std::size_t(i) * entrySize;
        PackageStoreEntry& entry = entries[first + i];
        if (version < EIoContainerHeaderVersion::NoExportInfo) {
            std::memcpy(&entry.exportCount, blob.data() + at, 4);
            std::memcpy(&entry.exportBundleCount, blob.data() + at + 4, 4);
            at += 8;
        }
        std::int32_t count;
        std::int32_t offset;
        std::memcpy(&count, blob.data() + at, 4);
        std::memcpy(&offset, blob.data() + at + 4, 4);
        if (count < 0 || (count > 0 && (offset < 0 || static_cast<std::size_t>(offset) > blob.size() - at ||
                                        std::size_t(count) * sizeof(FPackageId) > blob.size() - at - offset)))
            return fail(ErrorCode::InvalidData, "readPackageStore.imports");
        importRanges[first + i] = {imports.size(), static_cast<std::size_t>(count)};
        for (std::int32_t k = 0; k < count; k++) {
            FPackageId id;
            std::memcpy(&id.value, blob.data() + at + offset + std::size_t(k) * 8, 8);
            imports.push_back(id);
        }
        std::uint64_t id;
        std::memcpy(&id, ids.data() + std::size_t(i) * 8, 8);
        byId.try_emplace(id, static_cast<std::uint32_t>(first + i));
    }
    return {};
}

}

Result<PackageStore> readPackageStore(const FileSystem& files, std::stop_token stop) {
    PackageStore store;
    std::size_t archiveCount = files.getArchiveCount();
    store.containers.resize(archiveCount);
    std::vector<std::optional<Error>> errors(archiveCount);

    tbb::parallel_for(std::size_t(0), archiveCount, [&](std::size_t archiveIndex) {
        if (stop.stop_requested()) return;
        const MountedArchive& archive = files.getArchive(archiveIndex);
        if (archive.ioStore == nullptr) return;
        const IoStoreToc& toc = archive.ioStore->getToc();
        std::int64_t chunk = toc.findChunk(FIoChunkId::create(toc.getContainerId(), 0, EIoChunkType::ContainerHeader));
        if (chunk < 0) return;   // global.utoc, or a container without packages

        auto container = std::make_unique<PackageStore::Container>();
        container->data.resize(archive.ioStore->getChunkSize(static_cast<std::size_t>(chunk)));
        Result<void> read = archive.ioStore->read(static_cast<std::size_t>(chunk), 0, container->data, stop);
        if (!read) {
            errors[archiveIndex] = read.error();
            return;
        }

        ByteReader reader{container->data};
        std::uint32_t signature = reader.read<std::uint32_t>();
        EIoContainerHeaderVersion version = static_cast<EIoContainerHeaderVersion>(reader.read<std::uint32_t>());
        reader.skip(8);   // ContainerId
        if (!reader.hasFailed() && version < EIoContainerHeaderVersion::OptionalSegmentPackages) reader.skip(4);
        if (reader.hasFailed()) {
            errors[archiveIndex] = reader.getError();
            return;
        }
        if (signature != containerHeaderSignature) {
            errors[archiveIndex] = Error{ErrorCode::BadMagic, "readPackageStore.signature", 0};
            return;
        }
        if (version > latestContainerHeaderVersion) {
            errors[archiveIndex] = Error{ErrorCode::UnsupportedVersion, "readPackageStore.version", 0};
            return;
        }
        std::vector<std::pair<std::size_t, std::size_t>> ranges;
        Result<void> main = readStoreEntries(reader, version, container->entries, ranges, container->imports, container->byId);
        if (main && version >= EIoContainerHeaderVersion::OptionalSegmentPackages)
            main = readStoreEntries(reader, version, container->entries, ranges, container->imports, container->byId);
        if (!main) {
            errors[archiveIndex] = main.error();
            return;
        }
        // The spans are made once the vector stops growing.
        for (std::size_t i = 0; i < ranges.size(); i++)
            container->entries[i].importedPackages = std::span(container->imports).subspan(ranges[i].first, ranges[i].second);
        container->data = {};
        store.containers[archiveIndex] = std::move(container);
    });
    if (stop.stop_requested()) return fail(ErrorCode::Cancelled, "readPackageStore");

    for (std::size_t i = 0; i < archiveCount; i++)
        if (errors[i]) store.problems.push_back(ArchiveProblem{files.getArchive(i).name, *errors[i]});
    // One array of all entries, so that each package has a dense index (design §7.5).
    for (std::unique_ptr<PackageStore::Container>& container : store.containers) {
        if (container == nullptr) continue;
        std::uint32_t base = static_cast<std::uint32_t>(store.entries.size());
        for (auto& entry : container->byId) entry.second += base;
        store.entries.insert(store.entries.end(), container->entries.begin(), container->entries.end());
        container->entries = {};
    }
    return store;
}

}
