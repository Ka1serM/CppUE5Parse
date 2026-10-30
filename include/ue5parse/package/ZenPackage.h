#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "ue5parse/core/Arena.h"
#include "ue5parse/core/FName.h"
#include "ue5parse/core/Result.h"
#include "ue5parse/core/Versions.h"
#include "ue5parse/package/PackageStore.h"
#include "ue5parse/package/ZenTypes.h"

namespace ue5parse {

// The parsed header of one Zen package (FZenPackageHeader::MakeView): its names, imports, exports, and where each
// export's data is. Owns its bytes. Immutable, so it is thread-safe.
class ZenPackageHeader {
public:
    ZenPackageHeader();
    ZenPackageHeader(ZenPackageHeader&&) noexcept;
    ZenPackageHeader& operator=(ZenPackageHeader&&) noexcept;
    ~ZenPackageHeader();

    FName getName() const { return name; }
    std::uint32_t getPackageFlags() const { return packageFlags; }
    // The bytes of the header; export data starts here in the package chunk.
    std::uint32_t getHeaderSize() const { return headerSize; }
    // The header size of the cooked .uasset, which positions in export data count from (bulk data offsets).
    std::uint32_t getCookedHeaderSize() const { return cookedHeaderSize; }
    // The file version: from the versioning info, or the latest of the engine release for unversioned packages.
    EUnrealEngineObjectUE5Version getFileVersionUE5() const { return fileVersionUE5; }
    bool isUnversioned() const { return unversioned; }

    std::span<const FName> getNameMap() const { return names; }
    FName getName(FMappedName mapped, std::span<const FName> globalNames) const;
    std::span<const FPackageObjectIndex> getImportMap() const { return importMap; }
    std::span<const std::uint64_t> getImportedPublicExportHashes() const { return importedPublicExportHashes; }
    std::span<const FPackageId> getImportedPackages() const { return importedPackages; }
    std::span<const FExportMapEntry> getExportMap() const { return exportMap; }
    std::span<const FBulkDataMapEntry> getBulkDataMap() const { return bulkDataMap; }

    // Where an export's serialized data starts in the package chunk, and the position of that start in the cooked
    // file (what Tell() returns while UE serializes it).
    std::uint64_t getExportChunkOffset(std::size_t index) const { return exportChunkOffsets[index]; }
    std::uint64_t getExportCookedOffset(std::size_t index) const { return exportCookedOffsets[index]; }

private:
    friend Result<ZenPackageHeader> parseZenPackageHeader(std::vector<std::byte> bytes, EGame game,
                                                          const PackageStoreEntry* storeEntry);

    std::vector<std::byte> bytes;   // the ASCII names point into it
    std::unique_ptr<Arena> storage;  // UTF-16 names as UTF-8
    FName name;
    std::uint32_t packageFlags = 0;
    std::uint32_t headerSize = 0;
    std::uint32_t cookedHeaderSize = 0;
    EUnrealEngineObjectUE5Version fileVersionUE5 = EUnrealEngineObjectUE5Version::INITIAL_VERSION;
    bool unversioned = true;
    std::vector<FName> names;
    std::vector<FPackageObjectIndex> importMap;
    std::vector<std::uint64_t> importedPublicExportHashes;
    std::vector<FPackageId> importedPackages;
    std::vector<FExportMapEntry> exportMap;
    std::vector<FBulkDataMapEntry> bulkDataMap;
    std::vector<std::uint64_t> exportChunkOffsets;
    std::vector<std::uint64_t> exportCookedOffsets;
};

// The size of FZenPackageSummary of a release: the first bytes of every package chunk.
std::size_t getZenSummarySize(EGame game);
// HeaderSize of a summary; 0 when the bytes are too few.
std::uint32_t peekZenHeaderSize(std::span<const std::byte> summary);

// Parses a whole header ([0, HeaderSize) of the package chunk). The store entry gives the imported packages, and in
// UE 5.0 to 5.2 the export bundle count; it may be null for a package without one (editor-only data).
Result<ZenPackageHeader> parseZenPackageHeader(std::vector<std::byte> bytes, EGame game,
                                               const PackageStoreEntry* storeEntry);

}
