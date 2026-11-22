#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Core/Versions.h"
#include "CppUE5Parse/GameTypes/AceCombat7/AceCombat7Decrypt.h"
#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/VFS/FileSystem.h"

namespace ue5parse {

struct LegacyImport {
    FName classPackage;
    FName className;
    std::int32_t outerIndex = 0;
    FName objectName;
};

struct LegacyPackageHeader {
    PackageHeader common;
    std::vector<LegacyImport> imports;
};

struct LegacyPackageFiles {
    FileEntry header;
    std::optional<FileEntry> exports;
    std::optional<FileEntry> bulkData;
    std::optional<FileEntry> optionalBulkData;
    std::optional<FileEntry> memoryMappedBulkData;
};

std::optional<std::string> getLegacyPackageName(std::string_view path);

LegacyPackageFiles findLegacyPackageFiles(const FileSystem& fileSystem, FileEntry header, std::string_view headerPath);

//! Names found in nameReplacements are replaced by their value, as CUE4Parse does for PUBG.
Result<LegacyPackageHeader> parseLegacyPackageHeader(std::span<const std::byte> bytes, std::string_view packageName, EGame game,
                                                     const std::unordered_map<std::string, std::string>* nameReplacements = nullptr);

std::string_view getAceCombat7FileName(std::string_view path);

//! Decrypts uasset bytes read at the offset and puts back the package tag of the first 4 bytes.
void decryptAceCombat7Header(std::span<std::byte> bytes, std::uint64_t offset, AceCombat7Stream stream, std::span<const std::byte> key);

//! Decrypts uexp bytes read at the offset and puts back the package tag of the last 4 bytes.
void decryptAceCombat7Exports(std::span<std::byte> bytes, std::uint64_t offset, std::uint64_t headerSize, std::uint64_t exportsSize, AceCombat7Stream stream,
                              std::span<const std::byte> key);

class LegacyPackage final : public Package {
public:
    LegacyPackage(const FileProvider& provider, LegacyPackageFiles files, std::uint64_t headerFileSize, LegacyPackageHeader header,
                  std::optional<AceCombat7Stream> aceCombat7 = std::nullopt);
    ~LegacyPackage() override;

    ResolvedObject resolve(std::int32_t packageIndex, std::stop_token stop = {}) const override;
    bool resolveLoads(std::int32_t packageIndex) const override;
    FName getImportName(std::size_t importIndex) const override;
    std::int32_t getImportOuterIndex(std::size_t importIndex) const override;
    FName getImportClassName(std::size_t importIndex) const override;

    std::uint64_t getExportDataSize() const override;
    Result<void> readExportData(std::uint64_t offset, std::span<std::byte> out, std::stop_token stop = {}) const override;
    std::optional<std::uint64_t> getBulkDataSize(EIoChunkType type, std::uint8_t cookedIndex) const override;
    Result<void> readBulkData(EIoChunkType type, std::uint8_t cookedIndex, std::uint64_t offset, std::span<std::byte> out) const override;

private:
    std::optional<std::size_t> findTopImport(std::size_t importIndex) const;
    bool isNativeImport(std::size_t topImport) const;
    Result<const Package*> loadImportedPackage(std::size_t topImport, std::stop_token stop) const;
    std::string getImportPathBelow(std::size_t topImport, std::size_t importIndex) const;
    std::optional<FileEntry> findBulkData(EIoChunkType type, std::uint8_t cookedIndex) const;

    LegacyPackageFiles files;
    std::uint64_t headerFileSize;
    std::optional<AceCombat7Stream> aceCombat7;
    std::vector<LegacyImport> imports;
    std::unique_ptr<std::atomic<const Package*>[]> loaded;
};

}
