#include "CppUE5Parse/Package/ZenPackage.h"

#include <algorithm>
#include <cstring>
#include <format>

#include <ankerl/unordered_dense.h>
#include <oneapi/tbb/collaborative_call_once.h>

#include "CppUE5Parse/Provider/FileProvider.h"

namespace ue5parse {

namespace {

constexpr std::uint32_t exportCommandSerialize = 1;
constexpr std::uint32_t packageFilterEditorOnly = 0x80000000;

struct ZenSummary {
    std::uint32_t hasVersioningInfo;
    std::uint32_t headerSize;
    FMappedName name;
    std::uint32_t packageFlags;
    std::uint32_t cookedHeaderSize;
    std::int32_t importedPublicExportHashesOffset;
    std::int32_t importMapOffset;
    std::int32_t exportMapOffset;
    std::int32_t exportBundleEntriesOffset;
    std::int32_t graphDataOffset = 0;
    std::int32_t dependencyBundleHeadersOffset = 0;
    std::int32_t dependencyBundleEntriesOffset = 0;
    std::int32_t importedPackageNamesOffset = 0;
};

struct IdHash {
    using is_avalanching = void;
    std::uint64_t operator()(std::uint64_t value) const noexcept { return value; }
};

bool isRange(std::int64_t begin, std::int64_t end, std::size_t elementSize, std::size_t limit) {
    return begin >= 0 && end >= begin && static_cast<std::size_t>(end) <= limit && (static_cast<std::size_t>(end - begin) % elementSize) == 0;
}

template <class T>
std::vector<T> copyRange(std::span<const std::byte> bytes, std::int64_t begin, std::int64_t end) {
    std::vector<T> out(static_cast<std::size_t>(end - begin) / sizeof(T));
    if (!out.empty())
        std::memcpy(out.data(), bytes.data() + begin, out.size() * sizeof(T));
    return out;
}

Result<std::vector<FName>> readImportedPackageNames(std::span<const std::byte> bytes, Arena& storage) {
    ByteReader reader{bytes};
    Result<std::vector<FName>> entries = readNameBatch(reader, storage);
    if (!entries)
        return entries;
    std::vector<FName> names(entries->size());
    for (std::size_t i = 0; i < names.size(); i++)
        names[i] = makeName(*entries, static_cast<std::uint32_t>(i), reader.read<std::uint32_t>());
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    return names;
}

std::size_t getZenSummarySize(EGame game) {
    return game >= EGame::GAME_UE5_3 ? 52 : 44;
}

class PackageIndexMapper {
public:
    explicit PackageIndexMapper(std::vector<FPackageObjectIndex>& imports) : imports(imports) {
        for (std::size_t i = 0; i < imports.size(); i++)
            byValue.try_emplace(imports[i].value, static_cast<std::int32_t>(i));
    }

    std::int32_t map(FPackageObjectIndex index) {
        if (index.isNull())
            return 0;
        if (index.isExport())
            return static_cast<std::int32_t>(index.getIndex()) + 1;
        auto [found, inserted] = byValue.try_emplace(index.value, static_cast<std::int32_t>(imports.size()));
        if (inserted)
            imports.push_back(index);
        return -(found->second + 1);
    }

private:
    std::vector<FPackageObjectIndex>& imports;
    ankerl::unordered_dense::map<std::uint64_t, std::int32_t, IdHash> byValue;
};

Result<void> readExportOffsets(std::span<const std::byte> data, const ZenSummary& summary, EGame game, const PackageStoreEntry* storeEntry,
                               std::span<const FExportMapEntry> exportMap, std::vector<PackageExport>& exports) {
    if (game >= EGame::GAME_UE5_3) {
        for (std::size_t i = 0; i < exportMap.size(); i++) {
            exports[i].dataOffset = summary.headerSize + exportMap[i].cookedSerialOffset;
            exports[i].cookedOffset = summary.cookedHeaderSize + exportMap[i].cookedSerialOffset;
        }
        return {};
    }
    std::uint32_t bundleCount = storeEntry != nullptr ? storeEntry->exportBundleCount : 1;
    std::int64_t entriesEnd = summary.graphDataOffset;
    std::size_t limit = data.size();
    if (!isRange(summary.exportBundleEntriesOffset, entriesEnd, sizeof(FExportBundleEntry), limit)
        || !isRange(summary.graphDataOffset, summary.graphDataOffset + std::int64_t(bundleCount) * 16, 16, limit))
        return fail(ErrorCode::InvalidData, "parseZenPackageHeader.bundles");
    std::vector<FExportBundleEntry> entries = copyRange<FExportBundleEntry>(data, summary.exportBundleEntriesOffset, entriesEnd);
    std::vector<FExportBundleHeader> bundles =
        copyRange<FExportBundleHeader>(data, summary.graphDataOffset, summary.graphDataOffset + std::int64_t(bundleCount) * 16);
    for (const FExportBundleHeader& bundle : bundles) {
        if (bundle.firstEntryIndex > entries.size() || bundle.entryCount > entries.size() - bundle.firstEntryIndex)
            return fail(ErrorCode::InvalidData, "parseZenPackageHeader.bundle");
        std::uint64_t position = summary.headerSize + bundle.serialOffset;
        for (std::uint32_t i = 0; i < bundle.entryCount; i++) {
            const FExportBundleEntry& entry = entries[bundle.firstEntryIndex + i];
            if (entry.commandType != exportCommandSerialize)
                continue;
            if (entry.localExportIndex >= exportMap.size())
                return fail(ErrorCode::InvalidData, "parseZenPackageHeader.bundleEntry");
            exports[entry.localExportIndex].dataOffset = position;
            exports[entry.localExportIndex].cookedOffset = exportMap[entry.localExportIndex].cookedSerialOffset;
            position += exportMap[entry.localExportIndex].cookedSerialSize;
        }
    }
    return {};
}

}

std::uint32_t peekZenHeaderSize(std::span<const std::byte> summary) {
    if (summary.size() < 8)
        return 0;
    std::uint32_t size;
    std::memcpy(&size, summary.data() + 4, 4);
    return size;
}

Result<ZenPackageHeader> parseZenPackageHeader(std::span<const std::byte> data, EGame game, const PackageStoreEntry* storeEntry,
                                               std::span<const FName> globalNames) {
    ZenPackageHeader header;
    PackageHeader& common = header.common;
    ByteReader reader{data};

    ZenSummary summary;
    summary.hasVersioningInfo = reader.read<std::uint32_t>();
    summary.headerSize = reader.read<std::uint32_t>();
    summary.name = reader.read<FMappedName>();
    summary.packageFlags = reader.read<std::uint32_t>();
    summary.cookedHeaderSize = reader.read<std::uint32_t>();
    summary.importedPublicExportHashesOffset = reader.read<std::int32_t>();
    summary.importMapOffset = reader.read<std::int32_t>();
    summary.exportMapOffset = reader.read<std::int32_t>();
    summary.exportBundleEntriesOffset = reader.read<std::int32_t>();
    if (game >= EGame::GAME_UE5_3) {
        summary.dependencyBundleHeadersOffset = reader.read<std::int32_t>();
        summary.dependencyBundleEntriesOffset = reader.read<std::int32_t>();
        summary.importedPackageNamesOffset = reader.read<std::int32_t>();
    } else {
        summary.graphDataOffset = reader.read<std::int32_t>();
    }
    if (reader.hasFailed())
        return std::unexpected(reader.getError());
    if (summary.headerSize > data.size() || summary.headerSize < getZenSummarySize(game))
        return fail(ErrorCode::InvalidData, "parseZenPackageHeader.headerSize");
    std::span<std::byte> owned = common.storage->allocate<std::byte>(summary.headerSize);
    std::memcpy(owned.data(), data.data(), owned.size());
    data = owned;
    reader = ByteReader{data};
    reader.seek(getZenSummarySize(game));

    common.unversionedProperties = summary.hasVersioningInfo == 0;
    common.unversioned = summary.hasVersioningInfo == 0;
    common.filterEditorOnly = (summary.packageFlags & packageFilterEditorOnly) != 0;
    common.fileVersion = getPackageFileVersion(game);
    if (summary.hasVersioningInfo != 0) {
        reader.skip(sizeof(std::uint32_t));
        common.fileVersion = reader.read<FPackageFileVersion>();
        reader.skip(sizeof(std::int32_t));
        common.customVersions.resize(reader.readCount(sizeof(FCustomVersion)));
        for (FCustomVersion& customVersion : common.customVersions)
            customVersion = reader.read<FCustomVersion>();
        std::ranges::sort(common.customVersions, {}, &FCustomVersion::key);
    }
    using V = EUnrealEngineObjectUE5Version;

    std::int64_t cellImportMapOffset = summary.exportBundleEntriesOffset;
    std::int64_t cellExportMapOffset = summary.exportBundleEntriesOffset;
    if (game >= EGame::GAME_UE5_6 && common.fileVersion >= V::VERSE_CELLS) {
        cellImportMapOffset = reader.read<std::int32_t>();
        cellExportMapOffset = reader.read<std::int32_t>();
    }

    Result<std::vector<FName>> names = readNameBatch(reader, *common.storage);
    if (!names)
        return std::unexpected(names.error());
    common.names = std::move(*names);
    common.name = makeName(common.names, summary.name.getIndex(), summary.name.number);

    if ((game >= EGame::GAME_UE5_2 && common.fileVersion >= V::DATA_RESOURCES) || game == EGame::GAME_TheFirstDescendant) {
        if (game >= EGame::GAME_UE5_4) {
            std::uint64_t pad = reader.read<std::uint64_t>();
            if (pad > 8)
                return fail(ErrorCode::InvalidData, "parseZenPackageHeader.bulkDataPad");
            reader.skip(static_cast<std::size_t>(pad));
        }
        std::int64_t size = reader.read<std::int64_t>();
        if (reader.hasFailed())
            return std::unexpected(reader.getError());
        if (size < 0 || static_cast<std::uint64_t>(size) > reader.getRemaining() || size % sizeof(FBulkDataMapEntry) != 0)
            return fail(ErrorCode::InvalidData, "parseZenPackageHeader.bulkDataMap");
        std::size_t at = reader.getPosition();
        common.bulkDataMap = copyRange<FBulkDataMapEntry>(data, static_cast<std::int64_t>(at), static_cast<std::int64_t>(at) + size);
    }
    if (reader.hasFailed())
        return std::unexpected(reader.getError());

    std::size_t limit = data.size();
    if (!isRange(summary.importedPublicExportHashesOffset, summary.importMapOffset, 8, limit)
        || !isRange(summary.importMapOffset, summary.exportMapOffset, 8, limit)
        || !isRange(summary.exportMapOffset, cellImportMapOffset, sizeof(FExportMapEntry), limit)
        || !isRange(cellImportMapOffset, cellExportMapOffset, 8, limit) || !isRange(cellExportMapOffset, summary.exportBundleEntriesOffset, 1, limit))
        return fail(ErrorCode::InvalidData, "parseZenPackageHeader.offsets");
    header.importedPublicExportHashes = copyRange<std::uint64_t>(data, summary.importedPublicExportHashesOffset, summary.importMapOffset);
    header.imports = copyRange<FPackageObjectIndex>(data, summary.importMapOffset, summary.exportMapOffset);
    std::vector<FExportMapEntry> exportMap = copyRange<FExportMapEntry>(data, summary.exportMapOffset, cellImportMapOffset);
    if (storeEntry != nullptr)
        header.importedPackages.assign(storeEntry->importedPackages.begin(), storeEntry->importedPackages.end());

    if (summary.importedPackageNamesOffset > 0 && static_cast<std::uint32_t>(summary.importedPackageNamesOffset) < summary.headerSize) {
        Result<std::vector<FName>> importedNames = readImportedPackageNames(data.subspan(static_cast<std::size_t>(summary.importedPackageNamesOffset)),
                                                                            *common.storage);
        if (!importedNames)
            return std::unexpected(importedNames.error());
        header.importedPackageNames = std::move(*importedNames);
    }

    PackageIndexMapper mapper(header.imports);
    common.exports.resize(exportMap.size());
    header.publicExportHashes.resize(exportMap.size());
    for (std::size_t i = 0; i < exportMap.size(); i++) {
        const FExportMapEntry& entry = exportMap[i];
        PackageExport& out = common.exports[i];
        std::span<const FName> nameSource = entry.objectName.isGlobal() ? globalNames : std::span<const FName>(common.names);
        out.name = makeName(nameSource, entry.objectName.getIndex(), entry.objectName.number);
        out.outerIndex = mapper.map(entry.outerIndex);
        out.classIndex = mapper.map(entry.classIndex);
        out.superIndex = mapper.map(entry.superIndex);
        out.templateIndex = mapper.map(entry.templateIndex);
        out.objectFlags = entry.objectFlags;
        out.serialSize = entry.cookedSerialSize;
        header.publicExportHashes[i] = entry.publicExportHash;
    }
    common.importCount = header.imports.size();
    Result<void> offsets = readExportOffsets(data, summary, game, storeEntry, exportMap, common.exports);
    if (!offsets)
        return std::unexpected(offsets.error());
    return header;
}

struct ZenPackage::HashLookup {
    tbb::collaborative_once_flag once;
    ankerl::unordered_dense::map<std::uint64_t, std::uint32_t, IdHash> byPublicHash;
};

ZenPackage::ZenPackage(const FileProvider& provider, ChunkLocation chunk, FPackageId id, ZenPackageHeader header)
    : Package(provider, std::move(header.common)), chunk(chunk), id(id), imports(std::move(header.imports)),
      importedPublicExportHashes(std::move(header.importedPublicExportHashes)), importedPackages(std::move(header.importedPackages)),
      importedPackageNames(std::move(header.importedPackageNames)),       publicExportHashes(std::move(header.publicExportHashes)), imported(std::make_unique<std::atomic<const Package*>[]>(importedPackages.size())),
      hashLookup(std::make_unique<HashLookup>()) {}

ZenPackage::~ZenPackage() = default;

const ZenPackage::HashLookup& ZenPackage::getHashLookup() const {
    tbb::collaborative_call_once(hashLookup->once, [&] {
        hashLookup->byPublicHash.reserve(publicExportHashes.size());
        for (std::uint32_t i = 0; i < publicExportHashes.size(); i++)
            hashLookup->byPublicHash.try_emplace(publicExportHashes[i], i);
    });
    return *hashLookup;
}

std::int64_t ZenPackage::findExportByPublicHash(std::uint64_t publicExportHash) const {
    const HashLookup& lookup = getHashLookup();
    auto found = lookup.byPublicHash.find(publicExportHash);
    return found != lookup.byPublicHash.end() ? found->second : -1;
}

const FPackageObjectIndex* ZenPackage::findImport(std::int32_t packageIndex) const {
    if (packageIndex >= 0)
        return nullptr;
    std::size_t import = static_cast<std::size_t>(-(static_cast<std::int64_t>(packageIndex) + 1));
    return import < imports.size() ? &imports[import] : nullptr;
}

ResolvedObject ZenPackage::resolveImport(std::size_t importIndex, std::stop_token stop) const {
    const FPackageObjectIndex index = imports[importIndex];
    if (index.isExport()) {
        if (index.getIndex() >= getExportCount())
            return {};
        return ResolvedObject::makeExport(this, static_cast<std::uint32_t>(index.getIndex()));
    }
    if (index.isScriptImport()) {
        const FScriptObjectEntry* entry = getProvider().getScriptObjects().find(index);
        if (entry == nullptr)
            return {};
        return ResolvedObject::makeScript(&getProvider(), entry);
    }
    if (!index.isPackageImport())
        return {};
    std::uint32_t packageIndex = index.getImportedPackageIndex();
    std::uint32_t hashIndex = index.getImportedPublicExportHashIndex();
    if (packageIndex >= importedPackages.size() || hashIndex >= importedPublicExportHashes.size())
        return {};
    const Package* other = imported[packageIndex].load(std::memory_order_acquire);
    if (other == nullptr) {
        Result<const Package*> loaded = getProvider().loadPackage(importedPackages[packageIndex], stop);
        if (!loaded && loaded.error().code == ErrorCode::MissingPackage)
            return ResolvedObject::makeMissingImport(this, static_cast<std::uint32_t>(importIndex));
        if (!loaded)
            return {};
        other = *loaded;
        imported[packageIndex].store(other, std::memory_order_release);
    }
    std::int64_t exportIndex = other->findExportByPublicHash(importedPublicExportHashes[hashIndex]);
    if (exportIndex < 0)
        return {};
    return ResolvedObject::makeExport(other, static_cast<std::uint32_t>(exportIndex));
}

ResolvedObject ZenPackage::resolve(std::int32_t packageIndex, std::stop_token stop) const {
    if (packageIndex > 0) {
        if (static_cast<std::size_t>(packageIndex) > getExportCount())
            return {};
        return ResolvedObject::makeExport(this, static_cast<std::uint32_t>(packageIndex - 1));
    }
    const FPackageObjectIndex* import = findImport(packageIndex);
    return import != nullptr ? resolveImport(static_cast<std::size_t>(import - imports.data()), stop) : ResolvedObject();
}

void ZenPackage::appendImportPathName(std::size_t importIndex, std::string& out) const {
    if (importIndex >= imports.size() || !imports[importIndex].isPackageImport())
        return;
    const std::uint32_t packageIndex = imports[importIndex].getImportedPackageIndex();
    if (packageIndex < importedPackageNames.size()) {
        importedPackageNames[packageIndex].appendTo(out);
        return;
    }
    if (packageIndex < importedPackages.size())
        out += std::format("package id {:016X}", importedPackages[packageIndex].value);
}

bool ZenPackage::resolveLoads(std::int32_t packageIndex) const {
    const FPackageObjectIndex* import = findImport(packageIndex);
    if (import == nullptr || !import->isPackageImport())
        return false;
    const std::uint32_t imported = import->getImportedPackageIndex();
    if (imported >= importedPackages.size())
        return false;
    return this->imported[imported].load(std::memory_order_acquire) == nullptr;
}

std::uint64_t ZenPackage::getExportDataSize() const {
    return getProvider().getFiles().getChunkSize(chunk);
}

Result<void> ZenPackage::readExportData(std::uint64_t offset, std::span<std::byte> out, std::stop_token stop) const {
    std::uint64_t chunkSize = getExportDataSize();
    if (offset > chunkSize || out.size() > chunkSize - offset)
        return fail(ErrorCode::InvalidData, "ZenPackage.readExportData.range");
    return getProvider().getFiles().readChunk(chunk, offset, out, stop);
}

std::optional<ChunkLocation> ZenPackage::findBulkData(EIoChunkType type, std::uint8_t cookedIndex) const {
    return getProvider().getFiles().findChunk(FIoChunkId::create(id.value, cookedIndex, type));
}

std::optional<std::uint64_t> ZenPackage::getBulkDataSize(EIoChunkType type, std::uint8_t cookedIndex) const {
    std::optional<ChunkLocation> location = findBulkData(type, cookedIndex);
    if (!location)
        return std::nullopt;
    return getProvider().getFiles().getChunkSize(*location);
}

Result<void> ZenPackage::readBulkData(EIoChunkType type, std::uint8_t cookedIndex, std::uint64_t offset, std::span<std::byte> out) const {
    std::optional<ChunkLocation> location = findBulkData(type, cookedIndex);
    if (!location)
        return fail(ErrorCode::MissingChunk, "ZenPackage.readBulkData");
    std::uint64_t chunkSize = getProvider().getFiles().getChunkSize(*location);
    if (offset > chunkSize || out.size() > chunkSize - offset)
        return fail(ErrorCode::InvalidData, "ZenPackage.readBulkData.range");
    return getProvider().getFiles().readChunk(*location, offset, out);
}

}
