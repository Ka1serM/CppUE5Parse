#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "CppUE5Parse/Core/Arena.h"
#include "CppUE5Parse/Core/ByteReader.h"
#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Result.h"

namespace ue5parse {

struct FPackageId {
    std::uint64_t value = 0;

    static FPackageId fromName(std::string_view packageName);
    bool operator==(const FPackageId&) const = default;
};

struct FMappedName {
    static constexpr std::uint32_t indexBits = 30;
    static constexpr std::uint32_t indexMask = (1u << indexBits) - 1;
    enum class EType : std::uint8_t { Package, Container, Global };

    std::uint32_t index = ~0u;
    std::uint32_t number = ~0u;

    std::uint32_t getIndex() const { return index & indexMask; }
    EType getType() const { return static_cast<EType>(index >> indexBits); }
    bool isGlobal() const { return (index >> indexBits) != 0; }
};
static_assert(sizeof(FMappedName) == 8);

struct FPackageObjectIndex {
    static constexpr std::uint64_t indexBits = 62;
    static constexpr std::uint64_t indexMask = (1ull << indexBits) - 1;
    enum class EType : std::uint8_t { Export, ScriptImport, PackageImport, Null };

    std::uint64_t value = ~0ull;

    EType getType() const { return static_cast<EType>(value >> indexBits); }
    bool isNull() const { return getType() == EType::Null; }
    bool isExport() const { return getType() == EType::Export; }
    bool isScriptImport() const { return getType() == EType::ScriptImport; }
    bool isPackageImport() const { return getType() == EType::PackageImport; }
    std::uint64_t getIndex() const { return value & indexMask; }
    std::uint32_t getImportedPackageIndex() const { return static_cast<std::uint32_t>((value & indexMask) >> 32); }
    std::uint32_t getImportedPublicExportHashIndex() const { return static_cast<std::uint32_t>(value); }
    bool operator==(const FPackageObjectIndex&) const = default;
};
static_assert(sizeof(FPackageObjectIndex) == 8);

struct FExportMapEntry {
    std::uint64_t cookedSerialOffset;
    std::uint64_t cookedSerialSize;
    FMappedName objectName;
    FPackageObjectIndex outerIndex;
    FPackageObjectIndex classIndex;
    FPackageObjectIndex superIndex;
    FPackageObjectIndex templateIndex;
    std::uint64_t publicExportHash;
    std::uint32_t objectFlags;
    std::uint8_t filterFlags;
    std::uint8_t pad[3];
};
static_assert(sizeof(FExportMapEntry) == 72);

struct FScriptObjectEntry {
    FMappedName objectName;
    FPackageObjectIndex globalIndex;
    FPackageObjectIndex outerIndex;
    FPackageObjectIndex cdoClassIndex;
};
static_assert(sizeof(FScriptObjectEntry) == 32);

struct FBulkDataMapEntry {
    std::int64_t serialOffset;
    std::int64_t duplicateSerialOffset;
    std::int64_t serialSize;
    std::uint32_t flags;
    std::uint32_t pad;
};
static_assert(sizeof(FBulkDataMapEntry) == 32);

struct FExportBundleEntry {
    std::uint32_t localExportIndex;
    std::uint32_t commandType;
};

struct FExportBundleHeader {
    std::uint64_t serialOffset;
    std::uint32_t firstEntryIndex;
    std::uint32_t entryCount;
};
static_assert(sizeof(FExportBundleHeader) == 16);

Result<std::vector<FName>> readNameBatch(ByteReader& reader, Arena& storage);

FName makeName(std::span<const FName> names, std::uint32_t index, std::uint32_t number);

}
