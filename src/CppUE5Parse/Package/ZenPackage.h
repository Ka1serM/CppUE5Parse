#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "CppUE5Parse/Core/Arena.h"
#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Core/Versions.h"
#include "CppUE5Parse/Package/PackageStore.h"
#include "CppUE5Parse/Package/ZenTypes.h"

namespace ue5parse {

class ZenPackageHeader {
public:
    ZenPackageHeader();
    ZenPackageHeader(ZenPackageHeader&&) noexcept;
    ZenPackageHeader& operator=(ZenPackageHeader&&) noexcept;
    ~ZenPackageHeader();

    FName getName() const { return name; }
    std::uint32_t getHeaderSize() const { return headerSize; }
    std::uint32_t getCookedHeaderSize() const { return cookedHeaderSize; }
    EUnrealEngineObjectUE5Version getFileVersionUE5() const { return fileVersionUE5; }
    bool isUnversioned() const { return unversioned; }

    std::span<const FName> getNameMap() const { return names; }
    FName getName(FMappedName mapped, std::span<const FName> globalNames) const;
    std::span<const FPackageObjectIndex> getImportMap() const { return importMap; }
    std::span<const std::uint64_t> getImportedPublicExportHashes() const { return importedPublicExportHashes; }
    std::span<const FPackageId> getImportedPackages() const { return importedPackages; }
    std::span<const FExportMapEntry> getExportMap() const { return exportMap; }
    std::span<const FBulkDataMapEntry> getBulkDataMap() const { return bulkDataMap; }

    std::uint64_t getExportChunkOffset(std::size_t index) const { return exportChunkOffsets[index]; }
    std::uint64_t getExportCookedOffset(std::size_t index) const { return exportCookedOffsets[index]; }

private:
    friend Result<ZenPackageHeader> parseZenPackageHeader(std::vector<std::byte> bytes, EGame game,
                                                          const PackageStoreEntry* storeEntry);

    std::vector<std::byte> bytes;
    std::unique_ptr<Arena> storage;
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

std::size_t getZenSummarySize(EGame game);
std::uint32_t peekZenHeaderSize(std::span<const std::byte> summary);

Result<ZenPackageHeader> parseZenPackageHeader(std::vector<std::byte> bytes, EGame game,
                                               const PackageStoreEntry* storeEntry);

}
