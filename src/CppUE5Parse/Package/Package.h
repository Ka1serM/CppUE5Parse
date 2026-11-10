#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Package/ResolvedObject.h"
#include "CppUE5Parse/Package/ZenPackage.h"
#include "CppUE5Parse/VFS/FileSystem.h"

namespace ue5parse {

class FileProvider;

class Package {
public:
    Package(const FileProvider& provider, ChunkLocation chunk, FPackageId id, ZenPackageHeader header);
    ~Package();
    Package(const Package&) = delete;
    Package& operator=(const Package&) = delete;

    const FileProvider& getProvider() const { return *provider; }
    const ZenPackageHeader& getHeader() const { return header; }
    FPackageId getId() const { return id; }
    FName getName() const { return header.getName(); }

    std::size_t getExportCount() const { return header.getExportMap().size(); }
    const FExportMapEntry& getExportMapEntry(std::size_t index) const { return header.getExportMap()[index]; }
    FName getExportName(std::size_t index) const;
    std::string getExportPathName(std::size_t index) const;
    void appendExportPathName(std::size_t index, std::string& out) const;
    std::int64_t findExportByPath(std::string_view objectPath) const;
    std::int64_t findExportByPublicHash(std::uint64_t publicExportHash) const;

    ResolvedObject resolve(FPackageObjectIndex index, std::stop_token stop = {}) const;
    ResolvedObject resolve(std::int32_t packageIndex, std::stop_token stop = {}) const;
    bool resolveLoads(FPackageObjectIndex index) const;
    bool resolveLoads(std::int32_t packageIndex) const;

    Result<std::vector<std::byte>> readExportBytes(std::size_t index, std::stop_token stop = {}) const;
    Result<void> readExportChunk(std::uint64_t offset, std::span<std::byte> out, std::stop_token stop = {}) const;
    Result<std::vector<std::byte>> readBulkData(EIoChunkType type, std::uint64_t offset, std::uint64_t size, std::stop_token stop = {}) const;

private:
    struct Lazy;

    const Lazy& getLookup() const;
    const Lazy& getPathLookup() const;

    const FileProvider* provider;
    ChunkLocation chunk;
    FPackageId id;
    ZenPackageHeader header;
    std::unique_ptr<Lazy> lazy;
};

}
