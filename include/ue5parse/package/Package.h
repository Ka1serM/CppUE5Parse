#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include "ue5parse/core/FName.h"
#include "ue5parse/core/Result.h"
#include "ue5parse/package/ResolvedObject.h"
#include "ue5parse/package/ZenPackage.h"
#include "ue5parse/vfs/FileSystem.h"

namespace ue5parse {

class FileProvider;

// One cooked Zen package: its header, its data (read on first use) and the lookups of its exports. The objects of the
// package are the runtime's (runtime::FLinkerLoad reads them from here). Owned by its FileProvider; thread-safe.
class Package {
public:
    Package(const FileProvider& provider, ChunkLocation chunk, FPackageId id, ZenPackageHeader header);
    ~Package();
    Package(const Package&) = delete;
    Package& operator=(const Package&) = delete;

    const FileProvider& getProvider() const { return *provider; }
    const ZenPackageHeader& getHeader() const { return header; }
    FPackageId getId() const { return id; }
    // "/Game/Maps/Ascent".
    FName getName() const { return header.getName(); }

    // The export map: header data only, so nothing is read.
    std::size_t getExportCount() const { return header.getExportMap().size(); }
    const FExportMapEntry& getExportMapEntry(std::size_t index) const { return header.getExportMap()[index]; }
    FName getExportName(std::size_t index) const;
    // "/Game/A/B.B:Sub" without loading the export.
    std::string getExportPathName(std::size_t index) const;
    void appendExportPathName(std::size_t index, std::string& out) const;
    // The index of the first export of that name, preferring a top-level one; -1 when there is none.
    std::int64_t findExport(FName name) const;
    // The export of a path inside this package: "B", "B:Sub", "B:Sub.Inner". Ignores case. -1 when there is none.
    std::int64_t findExportByPath(std::string_view objectPath) const;
    std::int64_t findExportByPublicHash(std::uint64_t publicExportHash) const;

    // What an index of this package points to: an export, a script object or an import of another package. An import
    // loads the other package's header.
    ResolvedObject resolve(FPackageObjectIndex index, std::stop_token stop = {}) const;
    // A serialized FPackageIndex: positive export + 1, negative import.
    ResolvedObject resolve(std::int32_t packageIndex, std::stop_token stop = {}) const;

    // The serialized bytes of an export. Reads the whole package data on first use.
    Result<std::span<const std::byte>> getExportBytes(std::size_t index, std::stop_token stop = {}) const;
    // The bulk data chunk of this package (texture mips): ExportBundleData's siblings BulkData and OptionalBulkData.
    Result<std::vector<std::byte>> readBulkData(EIoChunkType type, std::uint64_t offset, std::uint64_t size,
                                                std::stop_token stop = {}) const;

private:
    struct Lazy;

    Result<void> loadData() const;
    const Lazy& getLookup() const;

    const FileProvider* provider;
    ChunkLocation chunk;
    FPackageId id;
    ZenPackageHeader header;
    std::unique_ptr<Lazy> lazy;
};

}
