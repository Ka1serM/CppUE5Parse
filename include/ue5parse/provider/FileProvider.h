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
#include "ue5parse/core/Versions.h"
#include "ue5parse/package/PackageStore.h"
#include "ue5parse/schema/Mappings.h"
#include "ue5parse/vfs/FileSystem.h"

namespace ue5parse {

class Package;

// Game-specific layout changes, each named and documented (design §9). The app sets them for its game; the library
// branches on a quirk, never on a game.
enum GameQuirk : std::uint32_t {
    // ULightComponentBase (so sky lights too): 24 more bytes after the properties (a zero FVector, 1.0f, -1, 1.0f).
    QUIRK_LightComponentPadding = 1u << 0,
    // UInstancedStaticMeshComponent: 4 more bytes at the end of its own data.
    QUIRK_InstancedStaticMeshPadding = 1u << 1,
    // UMaterialInstance without a static permutation resource: 8 more bytes (0.0f and 1.0f).
    QUIRK_MaterialInstancePadding = 1u << 2,
    // FMaterialResource (shader maps): 8 more bytes.
    QUIRK_MaterialResourcePadding = 1u << 3,
    // UMapBuildDataRegistry: one more array.
    QUIRK_MapBuildDataArray = 1u << 4,
};

// The quirks of VALORANT (UE 5.3 with Riot's changes), as CUE4Parse's GAME_Valorant branches.
constexpr std::uint32_t valorantQuirks = QUIRK_LightComponentPadding | QUIRK_InstancedStaticMeshPadding |
                                         QUIRK_MaterialInstancePadding | QUIRK_MaterialResourcePadding | QUIRK_MapBuildDataArray;

// What a mount needs (design §7.2).
struct MountOptions {
    std::vector<std::unique_ptr<FileSource>> archives;   // the files of the Paks folder: .utoc, .ucas, .pak
    std::unique_ptr<Decryptor> decryptor;                // null when nothing is encrypted
    std::unique_ptr<Codecs> codecs;                      // null for the defaults
    std::unique_ptr<FileSource> mappings;                // .usmap; null for a game without unversioned packages
    EngineVersion engine = EngineVersion::UE5_3;
    std::uint32_t quirks = 0;   // GameQuirk flags
};

// All the fallible work of a mount, done: the files, the mappings, the script objects and the package store.
struct MountedGame {
    FileSystem files;
    Mappings mappings;
    ScriptObjects scriptObjects;
    PackageStore packageStore;
    EngineVersion engine;
    std::uint32_t quirks = 0;
};

// Reads the archives, the mappings, the script objects and the container headers.
Result<MountedGame> mount(MountOptions options, std::stop_token stop = {});

// Owns everything of one mounted game: files, mappings, script objects and every loaded package. It must outlive all
// packages that it gives out and the runtime objects loaded from them (runtime::InitializeRuntime), and it does not
// move, because they point back to it. All methods are thread-safe.
class FileProvider {
public:
    explicit FileProvider(MountedGame game);
    ~FileProvider();
    FileProvider(const FileProvider&) = delete;
    FileProvider& operator=(const FileProvider&) = delete;

    const FileSystem& getFiles() const { return game.files; }
    const Mappings& getMappings() const { return game.mappings; }
    const ScriptObjects& getScriptObjects() const { return game.scriptObjects; }
    const PackageStore& getPackageStore() const { return game.packageStore; }
    EngineVersion getEngine() const { return game.engine; }
    bool hasQuirk(GameQuirk quirk) const { return (game.quirks & quirk) != 0; }
    std::uint32_t getQuirks() const { return game.quirks; }

    // "/Game/Maps/Ascent", with or without an object part or extension: "/Game/A/B.B", "/Game/A/B.umap". Loads the
    // header; the exports are read on use. The same package object for each call.
    Result<const Package*> loadPackage(std::string_view path, std::stop_token stop = {}) const;
    Result<const Package*> loadPackage(FPackageId id, std::stop_token stop = {}) const;
    // The bytes of a file of the archives: "ShooterGame/AssetRegistry.bin". CUE4Parse's TryCreateReader.
    Result<std::vector<std::byte>> readFile(std::string_view path, std::stop_token stop = {}) const;

private:
    struct Cache;

    Result<std::unique_ptr<Package>> readPackage(FPackageId id, ChunkLocation location, const PackageStoreEntry& entry) const;

    MountedGame game;
    std::unique_ptr<Cache> cache;
};

}

