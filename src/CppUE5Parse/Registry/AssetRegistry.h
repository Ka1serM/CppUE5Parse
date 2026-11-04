#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "CppUE5Parse/Core/Arena.h"
#include "CppUE5Parse/Core/FName.h"
#include "CppUE5Parse/Core/Result.h"

namespace ue5parse {

class FileProvider;

struct FAssetData {
    FName packageName;
    FName packagePath;
    FName assetName;
    FName assetClass;
    std::vector<std::pair<FName, std::string>> tagsAndValues;
    std::vector<std::int32_t> chunkIds;
    std::uint32_t packageFlags = 0;

    std::string getObjectPath() const;
    const std::string* findTag(FName key) const;
};

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

}
