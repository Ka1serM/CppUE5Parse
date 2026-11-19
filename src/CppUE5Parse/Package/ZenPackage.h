#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <vector>

#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Core/Versions.h"
#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/Package/PackageStore.h"
#include "CppUE5Parse/Package/ZenTypes.h"
#include "CppUE5Parse/VFS/FileSystem.h"

namespace ue5parse {

struct ZenPackageHeader {
    PackageHeader common;
    std::vector<FPackageObjectIndex> imports;
    std::vector<std::uint64_t> importedPublicExportHashes;
    std::vector<FPackageId> importedPackages;
    std::vector<std::uint64_t> publicExportHashes;
};

std::uint32_t peekZenHeaderSize(std::span<const std::byte> summary);

Result<ZenPackageHeader> parseZenPackageHeader(std::span<const std::byte> bytes, EGame game, const PackageStoreEntry* storeEntry,
                                               std::span<const FName> globalNames);

class ZenPackage final : public Package {
public:
    ZenPackage(const FileProvider& provider, ChunkLocation chunk, FPackageId id, ZenPackageHeader header);
    ~ZenPackage() override;

    std::int64_t findExportByPublicHash(std::uint64_t publicExportHash) const override;

    ResolvedObject resolve(std::int32_t packageIndex, std::stop_token stop = {}) const override;
    bool resolveLoads(std::int32_t packageIndex) const override;

    std::uint64_t getExportDataSize() const override;
    Result<void> readExportData(std::uint64_t offset, std::span<std::byte> out, std::stop_token stop = {}) const override;
    std::optional<std::uint64_t> getBulkDataSize(EIoChunkType type, std::uint8_t cookedIndex) const override;
    Result<void> readBulkData(EIoChunkType type, std::uint8_t cookedIndex, std::uint64_t offset, std::span<std::byte> out) const override;

private:
    struct HashLookup;

    ResolvedObject resolveImport(FPackageObjectIndex index, std::stop_token stop) const;
    const FPackageObjectIndex* findImport(std::int32_t packageIndex) const;
    const HashLookup& getHashLookup() const;
    std::optional<ChunkLocation> findBulkData(EIoChunkType type, std::uint8_t cookedIndex) const;

    ChunkLocation chunk;
    FPackageId id;
    std::vector<FPackageObjectIndex> imports;
    std::vector<std::uint64_t> importedPublicExportHashes;
    std::vector<FPackageId> importedPackages;
    std::vector<std::uint64_t> publicExportHashes;
    std::unique_ptr<std::atomic<const Package*>[]> imported;
    std::unique_ptr<HashLookup> hashLookup;
};

}
