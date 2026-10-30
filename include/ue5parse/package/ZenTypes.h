#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "ue5parse/core/Arena.h"
#include "ue5parse/core/ByteReader.h"
#include "ue5parse/core/FName.h"
#include "ue5parse/core/Result.h"

namespace ue5parse {

// FPackageId: CityHash64 of the lowercase UTF-16 package name ("/Game/Maps/Ascent"), as FPackageId::FromName.
struct FPackageId {
    std::uint64_t value = 0;

    // The name without an extension or object part. Does not allocate.
    static FPackageId fromName(std::string_view packageName);
    bool operator==(const FPackageId&) const = default;
};

// FMappedName: an index into a name map (the package's, or the global one of the script objects), and the number.
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

// FPackageObjectIndex: an export of this package, a script object, an import of another package, or null.
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
    // A package import: which of the store entry's imported packages, and which of this package's imported public
    // export hashes.
    std::uint32_t getImportedPackageIndex() const { return static_cast<std::uint32_t>((value & indexMask) >> 32); }
    std::uint32_t getImportedPublicExportHashIndex() const { return static_cast<std::uint32_t>(value); }
    bool operator==(const FPackageObjectIndex&) const = default;
};
static_assert(sizeof(FPackageObjectIndex) == 8);

// FExportMapEntry: 72 bytes in every UE5 release, used in place.
struct FExportMapEntry {
    std::uint64_t cookedSerialOffset;
    std::uint64_t cookedSerialSize;
    FMappedName objectName;
    FPackageObjectIndex outerIndex;
    FPackageObjectIndex classIndex;
    FPackageObjectIndex superIndex;
    FPackageObjectIndex templateIndex;
    std::uint64_t publicExportHash;
    std::uint32_t objectFlags;   // EObjectFlags
    std::uint8_t filterFlags;    // EExportFilterFlags: 1 not for client, 2 not for server
    std::uint8_t pad[3];
};
static_assert(sizeof(FExportMapEntry) == 72);

// FScriptObjectEntry of the global ScriptObjects chunk: "/Script/Engine", "StaticMesh", "Default__StaticMesh", ...
struct FScriptObjectEntry {
    FMappedName objectName;
    FPackageObjectIndex globalIndex;
    FPackageObjectIndex outerIndex;
    FPackageObjectIndex cdoClassIndex;
};
static_assert(sizeof(FScriptObjectEntry) == 32);

// FBulkDataMapEntry (UE 5.2 and later).
struct FBulkDataMapEntry {
    std::int64_t serialOffset;
    std::int64_t duplicateSerialOffset;
    std::int64_t serialSize;
    std::uint32_t flags;
    std::uint32_t pad;
};
static_assert(sizeof(FBulkDataMapEntry) == 32);

// FExportBundleEntry: CommandType 0 is Create, 1 is Serialize.
struct FExportBundleEntry {
    std::uint32_t localExportIndex;
    std::uint32_t commandType;
};

// FExportBundleHeader (UE 5.0 to 5.2).
struct FExportBundleHeader {
    std::uint64_t serialOffset;
    std::uint32_t firstEntryIndex;
    std::uint32_t entryCount;
};
static_assert(sizeof(FExportBundleHeader) == 16);

// A name batch (LoadNameBatch): the names of a package, of the script objects or of a container header. UTF-16 names
// become UTF-8 in storage; ANSI names that are ASCII stay views into data, which must outlive the names.
Result<std::vector<FName>> readNameBatch(ByteReader& reader, Arena& storage);

// An FName as serialized in Zen data: an index into names, and the number (0: no number).
FName makeName(std::span<const FName> names, std::uint32_t index, std::uint32_t number);

}
