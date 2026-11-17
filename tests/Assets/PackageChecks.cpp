#include <map>

#include "AssetSuite.h"
#include "CppUE5Parse/Registry/AssetRegistry.h"
#include "CppUE5Parse/VFS/FileSystem.h"

namespace test::assets {

using namespace ue5parse;

namespace {

constexpr std::string_view registryPath = "RefGen/AssetRegistry.bin";

void checkMount(const Cook& cook) {
    withProvider(cook, [](const FileProvider& provider) {
        for (const ArchiveProblem& problem : provider.getFiles().getProblems())
            printError(std::string(problem.name), problem.error);
        CHECK(provider.getFiles().getProblems().empty());
        CHECK(!provider.getFiles().getFiles().empty());
    });
}

void checkFiles(const Cook& cook) {
    withProvider(cook, [](const FileProvider& provider) {
        const FileSystem& files = provider.getFiles();
        std::vector<std::byte> bytes;
        for (const FileEntry& file : files.getFiles()) {
            bytes.resize(files.getFileSize(file));
            CHECK(files.readFile(file, 0, bytes).has_value());
        }
    });
}

void checkPackages(const Cook& cook) {
    withProvider(cook, [](const FileProvider& provider) { loadMainObjectPaths(provider); });
}

void checkObjects(const Cook& cook) {
    withRuntime(cook, [](const FileProvider& provider) {
        for (const std::string& objectPath : loadMainObjectPaths(provider))
            if (!CHECK(rt::StaticLoadObject(nullptr, nullptr, rt::FString(objectPath)) != nullptr))
                std::fprintf(stderr, "  cannot load %s\n", objectPath.c_str());
    });
}

void checkRegistry(const Cook& cook) {
    const Json* expectedAssets = cook.expected.find("assets");
    if (expectedAssets == nullptr) {
        fail(cook.version, "the reference values have no assets");
        return;
    }
    withProvider(cook, [&](const FileProvider& provider) {
        const FileSystem& files = provider.getFiles();
        std::optional<FileEntry> registryFile = files.findFile(registryPath);
        if (!registryFile)
            std::fprintf(stderr, "  the cook has no %.*s\n", static_cast<int>(registryPath.size()), registryPath.data());
        REQUIRE(registryFile.has_value());
        std::vector<std::byte> bytes(files.getFileSize(*registryFile));
        REQUIRE(files.readFile(*registryFile, 0, bytes).has_value());
        Result<FAssetRegistryState> registry = parseAssetRegistry(bytes);
        if (!registry)
            printError("asset registry", registry.error());
        REQUIRE(registry.has_value());
        std::map<std::string, std::string, std::less<>> classes;
        for (const FAssetData& asset : registry->getAssets())
            classes.emplace(asset.getObjectPath(), asset.assetClass.toString());
        for (const auto& [path, className] : expectedAssets->getObject()) {
            auto found = classes.find(path);
            if (found == classes.end())
                fail(path, "is not in the registry");
            else
                checkText(found->second, className.getString(), describe(path, "class"));
        }
    });
}

}

void addPackageChecks(Checks& checks) {
    checks.cook.emplace("mount", checkMount);
    checks.cook.emplace("files", checkFiles);
    checks.cook.emplace("packages", checkPackages);
    checks.cook.emplace("objects", checkObjects);
    checks.cook.emplace("registry", checkRegistry);
}

}
