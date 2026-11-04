#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stop_token>
#include <vector>

#include "CppUE5Parse/Core/Arena.h"
#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Result.h"
#include "CppUE5Parse/Package/ZenTypes.h"
#include "CppUE5Parse/VFS/FileSystem.h"

namespace ue5parse {

class ScriptObjects {
public:
    ScriptObjects();
    ScriptObjects(ScriptObjects&&) noexcept;
    ScriptObjects& operator=(ScriptObjects&&) noexcept;
    ~ScriptObjects();

    std::size_t getCount() const { return entries.size(); }
    const FScriptObjectEntry& getEntry(std::size_t index) const { return entries[index]; }
    const FScriptObjectEntry* find(FPackageObjectIndex index) const;
    FName getName(const FScriptObjectEntry& entry) const;
    const FScriptObjectEntry* getOuter(const FScriptObjectEntry& entry) const { return find(entry.outerIndex); }
    std::span<const FName> getNames() const { return names; }

private:
    friend Result<ScriptObjects> parseScriptObjects(std::span<const std::byte> chunk);
    struct Index;

    std::unique_ptr<Arena> storage;
    std::vector<std::byte> data;
    std::vector<FName> names;
    std::vector<FScriptObjectEntry> entries;
    std::unique_ptr<Index> index;
};

Result<ScriptObjects> parseScriptObjects(std::span<const std::byte> chunk);
Result<ScriptObjects> readScriptObjects(const FileSystem& files, std::stop_token stop = {});

struct PackageStoreEntry {
    std::span<const FPackageId> importedPackages;
    std::uint32_t exportCount = 0;
    std::uint32_t exportBundleCount = 0;
};

class PackageStore {
public:
    PackageStore();
    PackageStore(PackageStore&&) noexcept;
    PackageStore& operator=(PackageStore&&) noexcept;
    ~PackageStore();

    const PackageStoreEntry* find(std::uint32_t archive, FPackageId id) const;
    std::optional<std::uint32_t> findIndex(std::uint32_t archive, FPackageId id) const;
    const PackageStoreEntry& getEntry(std::uint32_t index) const { return entries[index]; }
    std::size_t getEntryCount() const { return entries.size(); }
    std::span<const ArchiveProblem> getProblems() const { return problems; }

private:
    friend Result<PackageStore> readPackageStore(const FileSystem& files, std::stop_token stop);
    struct Container;

    std::vector<std::unique_ptr<Container>> containers;
    std::vector<PackageStoreEntry> entries;
    std::vector<ArchiveProblem> problems;
};

Result<PackageStore> readPackageStore(const FileSystem& files, std::stop_token stop = {});

}
