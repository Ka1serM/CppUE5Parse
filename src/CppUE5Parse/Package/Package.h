#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Core/Arena.h"
#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Core/Versions.h"
#include "CppUE5Parse/Package/ResolvedObject.h"
#include "CppUE5Parse/Package/ZenTypes.h"

namespace ue5parse {

class FileProvider;
enum class EIoChunkType : std::uint8_t;

struct PackageExport {
    FName name;
    std::int32_t outerIndex = 0;
    std::int32_t classIndex = 0;
    std::int32_t superIndex = 0;
    std::int32_t templateIndex = 0;
    std::uint32_t objectFlags = 0;
    std::uint64_t serialSize = 0;
    std::uint64_t dataOffset = 0;
    std::uint64_t cookedOffset = 0;
};

struct PackageHeader {
    std::unique_ptr<Arena> storage = std::make_unique<Arena>(4096);
    FName name;
    FPackageFileVersion fileVersion;
    std::vector<FCustomVersion> customVersions;
    bool unversionedProperties = true;
    std::vector<FName> names;
    std::size_t importCount = 0;
    std::vector<PackageExport> exports;
    std::vector<FBulkDataMapEntry> bulkDataMap;
    std::int64_t bulkDataStartOffset = 0;
};

class Package {
public:
    virtual ~Package();
    Package(const Package&) = delete;
    Package& operator=(const Package&) = delete;

    const FileProvider& getProvider() const { return *provider; }
    FName getName() const { return header.name; }
    FPackageFileVersion getFileVersion() const { return header.fileVersion; }
    std::span<const FCustomVersion> getCustomVersions() const { return header.customVersions; }
    bool hasUnversionedProperties() const { return header.unversionedProperties; }
    std::span<const FName> getNameMap() const { return header.names; }
    std::size_t getImportCount() const { return header.importCount; }
    std::span<const FBulkDataMapEntry> getBulkDataMap() const { return header.bulkDataMap; }
    std::int64_t getBulkDataStartOffset() const { return header.bulkDataStartOffset; }

    std::size_t getExportCount() const { return header.exports.size(); }
    const PackageExport& getExport(std::size_t index) const { return header.exports[index]; }
    FName getExportName(std::size_t index) const { return header.exports[index].name; }
    std::string getExportPathName(std::size_t index) const;
    void appendExportPathName(std::size_t index, std::string& out) const;
    std::int64_t findExportByPath(std::string_view objectPath) const;
    virtual std::int64_t findExportByPublicHash(std::uint64_t publicExportHash) const { return -1; }

    virtual ResolvedObject resolve(std::int32_t packageIndex, std::stop_token stop = {}) const = 0;
    virtual bool resolveLoads(std::int32_t packageIndex) const = 0;
    virtual FName getImportName(std::size_t importIndex) const { return {}; }
    virtual std::int32_t getImportOuterIndex(std::size_t importIndex) const { return 0; }
    virtual FName getImportClassName(std::size_t importIndex) const { return {}; }
    virtual void appendImportPathName(std::size_t importIndex, std::string& out) const;

    Result<std::vector<std::byte>> readExportBytes(std::size_t index, std::stop_token stop = {}) const;
    virtual std::uint64_t getExportDataSize() const = 0;
    virtual Result<void> readExportData(std::uint64_t offset, std::span<std::byte> out, std::stop_token stop = {}) const = 0;
    virtual std::optional<std::uint64_t> getBulkDataSize(EIoChunkType type, std::uint8_t cookedIndex) const = 0;
    virtual Result<void> readBulkData(EIoChunkType type, std::uint8_t cookedIndex, std::uint64_t offset, std::span<std::byte> out) const = 0;

protected:
    Package(const FileProvider& provider, PackageHeader header);

private:
    struct PathLookup;

    const PathLookup& getPathLookup() const;

    const FileProvider* provider;
    PackageHeader header;
    std::unique_ptr<PathLookup> pathLookup;
};

}
