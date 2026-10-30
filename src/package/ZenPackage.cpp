#include <cstring>

#include "ue5parse/package/ZenPackage.h"

namespace ue5parse {

namespace {

constexpr std::uint32_t exportCommandSerialize = 1;

// FZenPackageSummary.
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
    std::int32_t graphDataOffset = 0;                 // UE 5.0 to 5.2
    std::int32_t dependencyBundleHeadersOffset = 0;   // UE 5.3 and later
    std::int32_t dependencyBundleEntriesOffset = 0;
    std::int32_t importedPackageNamesOffset = 0;
};

// A range [begin, end) of the header with whole elements of size elementSize.
bool isRange(std::int64_t begin, std::int64_t end, std::size_t elementSize, std::size_t limit) {
    return begin >= 0 && end >= begin && static_cast<std::size_t>(end) <= limit &&
           (static_cast<std::size_t>(end - begin) % elementSize) == 0;
}

template <class T>
std::vector<T> copyRange(std::span<const std::byte> bytes, std::int64_t begin, std::int64_t end) {
    std::vector<T> out(static_cast<std::size_t>(end - begin) / sizeof(T));
    if (!out.empty()) std::memcpy(out.data(), bytes.data() + begin, out.size() * sizeof(T));
    return out;
}

}

ZenPackageHeader::ZenPackageHeader() : storage(std::make_unique<Arena>(4096)) {}
ZenPackageHeader::ZenPackageHeader(ZenPackageHeader&&) noexcept = default;
ZenPackageHeader& ZenPackageHeader::operator=(ZenPackageHeader&&) noexcept = default;
ZenPackageHeader::~ZenPackageHeader() = default;

FName ZenPackageHeader::getName(FMappedName mapped, std::span<const FName> globalNames) const {
    return makeName(mapped.isGlobal() ? globalNames : std::span<const FName>(names), mapped.getIndex(), mapped.number);
}

std::size_t getZenSummarySize(EGame game) { return game >= EGame::GAME_UE5_3 ? 52 : 44; }

std::uint32_t peekZenHeaderSize(std::span<const std::byte> summary) {
    if (summary.size() < 8) return 0;
    std::uint32_t size;
    std::memcpy(&size, summary.data() + 4, 4);
    return size;
}

Result<ZenPackageHeader> parseZenPackageHeader(std::vector<std::byte> bytes, EGame game,
                                               const PackageStoreEntry* storeEntry) {
    ZenPackageHeader header;
    header.bytes = std::move(bytes);
    std::span<const std::byte> data = header.bytes;
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
    if (reader.hasFailed()) return std::unexpected(reader.getError());
    if (summary.headerSize > data.size() || summary.headerSize < getZenSummarySize(game))
        return fail(ErrorCode::InvalidData, "parseZenPackageHeader.headerSize");
    data = data.first(summary.headerSize);
    reader = ByteReader{data};
    reader.seek(getZenSummarySize(game));

    header.headerSize = summary.headerSize;
    header.cookedHeaderSize = summary.cookedHeaderSize;
    header.packageFlags = summary.packageFlags;
    header.unversioned = summary.hasVersioningInfo == 0;
    header.fileVersionUE5 = getLatestObjectVersion(game);
    if (!header.unversioned) {
        // FZenPackageVersioningInfo: ZenVersion, FPackageFileVersion (UE4, UE5), LicenseeVersion, custom versions.
        reader.skip(4 + 4);
        header.fileVersionUE5 = static_cast<EUnrealEngineObjectUE5Version>(reader.read<std::int32_t>());
        reader.skip(4);
        std::uint32_t customVersions = reader.readCount(20);
        reader.skip(std::size_t(customVersions) * 20);
    }
    using V = EUnrealEngineObjectUE5Version;

    std::int64_t cellImportMapOffset = summary.exportBundleEntriesOffset;
    std::int64_t cellExportMapOffset = summary.exportBundleEntriesOffset;
    if (game >= EGame::GAME_UE5_6 && header.fileVersionUE5 >= V::VERSE_CELLS) {
        cellImportMapOffset = reader.read<std::int32_t>();
        cellExportMapOffset = reader.read<std::int32_t>();
    }

    Result<std::vector<FName>> names = readNameBatch(reader, *header.storage);
    if (!names) return std::unexpected(names.error());
    header.names = std::move(*names);
    header.name = makeName(header.names, summary.name.getIndex(), summary.name.number);

    if (game >= EGame::GAME_UE5_2 && header.fileVersionUE5 >= V::DATA_RESOURCES) {
        if (game >= EGame::GAME_UE5_4) {
            std::uint64_t pad = reader.read<std::uint64_t>();
            if (pad > 8) return fail(ErrorCode::InvalidData, "parseZenPackageHeader.bulkDataPad");
            reader.skip(static_cast<std::size_t>(pad));
        }
        std::int64_t size = reader.read<std::int64_t>();
        if (reader.hasFailed()) return std::unexpected(reader.getError());
        if (size < 0 || static_cast<std::uint64_t>(size) > reader.getRemaining() || size % sizeof(FBulkDataMapEntry) != 0)
            return fail(ErrorCode::InvalidData, "parseZenPackageHeader.bulkDataMap");
        std::size_t at = reader.getPosition();
        header.bulkDataMap = copyRange<FBulkDataMapEntry>(data, static_cast<std::int64_t>(at), static_cast<std::int64_t>(at) + size);
    }
    if (reader.hasFailed()) return std::unexpected(reader.getError());

    std::size_t limit = data.size();
    if (!isRange(summary.importedPublicExportHashesOffset, summary.importMapOffset, 8, limit) ||
        !isRange(summary.importMapOffset, summary.exportMapOffset, 8, limit) ||
        !isRange(summary.exportMapOffset, cellImportMapOffset, sizeof(FExportMapEntry), limit) ||
        !isRange(cellImportMapOffset, cellExportMapOffset, 8, limit) ||
        !isRange(cellExportMapOffset, summary.exportBundleEntriesOffset, 1, limit))
        return fail(ErrorCode::InvalidData, "parseZenPackageHeader.offsets");
    header.importedPublicExportHashes =
        copyRange<std::uint64_t>(data, summary.importedPublicExportHashesOffset, summary.importMapOffset);
    header.importMap = copyRange<FPackageObjectIndex>(data, summary.importMapOffset, summary.exportMapOffset);
    header.exportMap = copyRange<FExportMapEntry>(data, summary.exportMapOffset, cellImportMapOffset);
    if (storeEntry != nullptr) header.importedPackages.assign(storeEntry->importedPackages.begin(), storeEntry->importedPackages.end());

    std::size_t exportCount = header.exportMap.size();
    header.exportChunkOffsets.assign(exportCount, 0);
    header.exportCookedOffsets.assign(exportCount, 0);
    if (game >= EGame::GAME_UE5_3) {
        // The serial offsets count from the end of the header.
        for (std::size_t i = 0; i < exportCount; i++) {
            const FExportMapEntry& entry = header.exportMap[i];
            header.exportChunkOffsets[i] = summary.headerSize + entry.cookedSerialOffset;
            header.exportCookedOffsets[i] = summary.cookedHeaderSize + entry.cookedSerialOffset;
        }
        return header;
    }

    // UE 5.0 to 5.2: the export bundles give the order of the export data. Each bundle starts at its serial offset
    // after the header, and its exports follow each other in entry order.
    std::uint32_t bundleCount = storeEntry != nullptr ? storeEntry->exportBundleCount : 1;
    std::int64_t entriesEnd = summary.graphDataOffset;
    if (!isRange(summary.exportBundleEntriesOffset, entriesEnd, sizeof(FExportBundleEntry), limit) ||
        !isRange(summary.graphDataOffset, summary.graphDataOffset + std::int64_t(bundleCount) * 16, 16, limit))
        return fail(ErrorCode::InvalidData, "parseZenPackageHeader.bundles");
    std::vector<FExportBundleEntry> entries =
        copyRange<FExportBundleEntry>(data, summary.exportBundleEntriesOffset, entriesEnd);
    std::vector<FExportBundleHeader> bundles =
        copyRange<FExportBundleHeader>(data, summary.graphDataOffset, summary.graphDataOffset + std::int64_t(bundleCount) * 16);
    for (const FExportBundleHeader& bundle : bundles) {
        if (bundle.firstEntryIndex > entries.size() || bundle.entryCount > entries.size() - bundle.firstEntryIndex)
            return fail(ErrorCode::InvalidData, "parseZenPackageHeader.bundle");
        std::uint64_t position = summary.headerSize + bundle.serialOffset;
        for (std::uint32_t i = 0; i < bundle.entryCount; i++) {
            const FExportBundleEntry& entry = entries[bundle.firstEntryIndex + i];
            if (entry.commandType != exportCommandSerialize) continue;
            if (entry.localExportIndex >= exportCount) return fail(ErrorCode::InvalidData, "parseZenPackageHeader.bundleEntry");
            header.exportChunkOffsets[entry.localExportIndex] = position;
            header.exportCookedOffsets[entry.localExportIndex] = header.exportMap[entry.localExportIndex].cookedSerialOffset;
            position += header.exportMap[entry.localExportIndex].cookedSerialSize;
        }
    }
    return header;
}

}
