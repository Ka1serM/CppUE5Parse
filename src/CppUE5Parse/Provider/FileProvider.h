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
#include "CppUE5Parse/Core/Versions.h"
#include "CppUE5Parse/Package/PackageStore.h"
#include "CppUE5Parse/Schema/Mappings.h"
#include "CppUE5Parse/VFS/FileSystem.h"

namespace ue5parse {

class FAssetRegistryState;
class Package;

struct MountOptions {
    std::vector<std::unique_ptr<FileSource>> archives;
    std::unique_ptr<Decryptor> decryptor;
    std::unique_ptr<Codecs> codecs;
    std::unique_ptr<FileSource> mappings;
    EGame game = EGame::GAME_UE5_LATEST;
};

struct MountedGame {
    FileSystem files;
    Mappings mappings;
    ScriptObjects scriptObjects;
    PackageStore packageStore;
    EGame game;
};

Result<MountedGame> mount(MountOptions options, std::stop_token stop = {});

//! Packages and the runtime point back to the provider: it must outlive them and cannot move.
class FileProvider {
public:
    explicit FileProvider(MountedGame game);
    ~FileProvider();
    FileProvider(const FileProvider&) = delete;
    FileProvider& operator=(const FileProvider&) = delete;

    const FileSystem& getFiles() const { return game.files; }
    const Mappings& getMappings() const { return game.mappings; }
    const ScriptObjects& getScriptObjects() const { return game.scriptObjects; }
    EGame getGame() const { return game.game; }

    Result<const Package*> loadPackage(std::string_view path, std::stop_token stop = {}) const;
    Result<const Package*> loadPackage(FPackageId id, std::stop_token stop = {}) const;
    Result<std::vector<std::byte>> readFile(std::string_view path, std::stop_token stop = {}) const;
    Result<const FAssetRegistryState*> getAssetRegistry(std::string_view path) const;

private:
    struct Cache;

    Result<std::unique_ptr<Package>> readPackage(FPackageId id, ChunkLocation location, const PackageStoreEntry& entry) const;

    MountedGame game;
    std::unique_ptr<Cache> cache;
};

}
