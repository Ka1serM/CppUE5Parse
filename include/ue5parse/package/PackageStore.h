#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <vector>

#include "ue5parse/core/Arena.h"
#include "ue5parse/core/FName.h"
#include "ue5parse/core/Result.h"
#include "ue5parse/package/ZenTypes.h"
#include "ue5parse/vfs/FileSystem.h"

namespace ue5parse {

// The ScriptObjects chunk of global.utoc: the native packages, classes, structs and default objects that packages
// import as "/Script/Engine.StaticMesh". Immutable, so it is thread-safe.
class ScriptObjects {
public:
    ScriptObjects();
    ScriptObjects(ScriptObjects&&) noexcept;
    ScriptObjects& operator=(ScriptObjects&&) noexcept;
    ~ScriptObjects();

    std::size_t getCount() const { return entries.size(); }
    const FScriptObjectEntry& getEntry(std::size_t index) const { return entries[index]; }
    // O(1). Null when the index is not a script object of this game.
    const FScriptObjectEntry* find(FPackageObjectIndex index) const;
    FName getName(const FScriptObjectEntry& entry) const;
    // The outer of an entry: a class's package, or null for a package.
    const FScriptObjectEntry* getOuter(const FScriptObjectEntry& entry) const { return find(entry.outerIndex); }
    // The global name map, which FMappedName of type Global indexes.
    std::span<const FName> getNames() const { return names; }

private:
    friend Result<ScriptObjects> parseScriptObjects(std::span<const std::byte> chunk);
    struct Index;

    std::unique_ptr<Arena> storage;
    std::vector<std::byte> data;   // the chunk, which the ASCII names point into
    std::vector<FName> names;
    std::vector<FScriptObjectEntry> entries;
    std::unique_ptr<Index> index;
};

Result<ScriptObjects> parseScriptObjects(std::span<const std::byte> chunk);
// Reads the ScriptObjects chunk, which only global.utoc has.
Result<ScriptObjects> readScriptObjects(const FileSystem& files, std::stop_token stop = {});

// FFilePackageStoreEntry of a container header: the packages that a package imports, in the order of its
// FPackageObjectIndex package imports.
struct PackageStoreEntry {
    std::span<const FPackageId> importedPackages;
    // UE 5.0 to 5.2 only: the export bundle count of the package, which its header does not have.
    std::uint32_t exportCount = 0;
    std::uint32_t exportBundleCount = 0;
};

// The container headers of every IoStore archive. A package's entry is looked up in the archive that holds its
// package chunk, so a patch container gives its own entry. Immutable, so it is thread-safe.
class PackageStore {
public:
    PackageStore();
    PackageStore(PackageStore&&) noexcept;
    PackageStore& operator=(PackageStore&&) noexcept;
    ~PackageStore();

    // O(1). Null when the archive has no entry for the package.
    const PackageStoreEntry* find(std::uint32_t archive, FPackageId id) const;
    // O(1). The dense index of the package's entry, below getEntryCount(); nullopt when the archive has none.
    std::optional<std::uint32_t> findIndex(std::uint32_t archive, FPackageId id) const;
    const PackageStoreEntry& getEntry(std::uint32_t index) const { return entries[index]; }
    std::size_t getEntryCount() const { return entries.size(); }
    std::span<const ArchiveProblem> getProblems() const { return problems; }

private:
    friend Result<PackageStore> readPackageStore(const FileSystem& files, std::stop_token stop);
    struct Container;

    std::vector<std::unique_ptr<Container>> containers;   // by archive index; null for paks and global.utoc
    std::vector<PackageStoreEntry> entries;               // of all containers, in archive order
    std::vector<ArchiveProblem> problems;                 // a container header that does not parse
};

// Reads and parses the container header of every IoStore archive, in parallel.
Result<PackageStore> readPackageStore(const FileSystem& files, std::stop_token stop = {});

}
