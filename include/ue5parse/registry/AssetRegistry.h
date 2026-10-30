#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "ue5parse/core/Arena.h"
#include "ue5parse/core/FName.h"
#include "ue5parse/core/Result.h"

namespace ue5parse {

class FileProvider;

// One asset of the registry (FAssetData).
struct FAssetData {
    FName packageName;    // "/Game/A/B"
    FName packagePath;    // "/Game/A"
    FName assetName;      // "B"
    FName assetClass;     // "BlueprintGeneratedClass": the class's short name
    std::vector<std::pair<FName, std::string>> tagsAndValues;
    std::vector<std::int32_t> chunkIds;
    std::uint32_t packageFlags = 0;

    // "/Game/A/B.B", as CUE4Parse's ObjectPath.
    std::string getObjectPath() const;
    // The value of a tag; null when the asset has no such tag.
    const std::string* findTag(FName key) const;
};

// The cooked asset registry (AssetRegistry.bin): the assets with their classes and tags. Dependencies and package
// data are not read. Owns its names.
class FAssetRegistryState {
public:
    FAssetRegistryState();
    FAssetRegistryState(FAssetRegistryState&&) noexcept;
    FAssetRegistryState& operator=(FAssetRegistryState&&) noexcept;
    ~FAssetRegistryState();

    std::int32_t getVersion() const { return version; }
    std::span<const FAssetData> getAssets() const { return assets; }

private:
    friend Result<FAssetRegistryState> parseAssetRegistry(std::span<const std::byte> file);

    std::unique_ptr<Arena> storage;
    std::vector<std::byte> bytes;
    std::int32_t version = 0;
    std::vector<FAssetData> assets;
};

Result<FAssetRegistryState> parseAssetRegistry(std::span<const std::byte> file);
// Reads a registry file of the archives, such as "ShooterGame/AssetRegistry.bin".
Result<FAssetRegistryState> readAssetRegistry(const FileProvider& provider, std::string_view path);

}
