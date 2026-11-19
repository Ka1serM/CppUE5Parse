#include <cstdio>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>

#include "CppUE5Parse/Crypto/Decryptor.h"
#include "CppUE5Parse/IO/FileSource.h"
#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/LinkerLoad.h"
#include "Engine/Source/Runtime/CoreUObject/UObject/UObjectGlobals.h"
#include "Engine/Source/Runtime/JsonUtilities/JsonObjectConverter.h"

using namespace ue5parse;
namespace Runtime = ue5parse::runtime;

namespace {

void printProblem(const char* code, const Runtime::FString& message) {
    std::fprintf(stderr, "%s: %s\n", code, message.ToUtf8().c_str());
}

std::optional<EGame> parseEngineVersion(std::string_view version) {
    const std::size_t dot = version.find('.');
    if (dot == std::string_view::npos)
        return std::nullopt;
    const std::string_view minor = version.substr(dot + 1, version.find('.', dot + 1) - dot - 1);
    return gameOfName("GAME_UE" + std::string(version.substr(0, dot)) + "_" + std::string(minor));
}

bool addDecryptor(MountOptions& options) {
    const char* key = std::getenv("UE5_AES_KEY");
    if (key == nullptr)
        return true;
    Result<AesKey> parsed = parseAesKey(key);
    if (!parsed) {
        std::fprintf(stderr, "UE5_AES_KEY is not a key\n");
        return false;
    }
    KeyRing keys;
    keys.add(FGuid{}, *parsed);
    options.decryptor = std::make_unique<AesDecryptor>(keys);
    return true;
}

bool addMappings(MountOptions& options) {
    const char* path = std::getenv("UE5_MAPPINGS");
    if (path == nullptr)
        return true;
    Result<std::unique_ptr<FileSource>> mappings = openLocalFile(path);
    if (!mappings) {
        std::fprintf(stderr, "cannot open %s: %s\n", path, getErrorName(mappings.error().code));
        return false;
    }
    options.mappings = std::move(*mappings);
    return true;
}

bool addArchives(MountOptions& options, int count, char** paths) {
    for (int i = 0; i < count; i++) {
        Result<std::unique_ptr<FileSource>> source = openLocalFile(paths[i]);
        if (!source) {
            std::fprintf(stderr, "cannot open %s: %s at %s\n", paths[i], getErrorName(source.error().code), source.error().where);
            return false;
        }
        options.archives.push_back(std::move(*source));
    }
    return true;
}

}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::fprintf(stderr, "usage: ue5json <engine version> <package path> <archive>...\n");
        return 2;
    }
    MountOptions options;
    const std::optional<EGame> game = parseEngineVersion(argv[1]);
    if (!game) {
        std::fprintf(stderr, "%s is not an engine version\n", argv[1]);
        return 2;
    }
    options.game = *game;
    if (!addDecryptor(options) || !addMappings(options) || !addArchives(options, argc - 3, argv + 3))
        return 1;
    Result<MountedGame> mounted = mount(std::move(options));
    if (!mounted) {
        std::fprintf(stderr, "mount failed: %s\n", getErrorName(mounted.error().code));
        return 1;
    }
    FileProvider provider(std::move(*mounted));
    Runtime::FRuntime runtime(provider);
    Runtime::SetLoadProblemHandler(&printProblem);
    Runtime::FLinkerLoad::SetKeepUnknownProperties(true);
    Result<const Package*> loaded = provider.loadPackage(argv[2]);
    if (!loaded) {
        std::fprintf(stderr, "cannot load %s: %s at %s\n", argv[2], getErrorName(loaded.error().code), loaded.error().where);
        return 1;
    }
    const Package& package = **loaded;
    Runtime::FLinkerLoad* linker = Runtime::FLinkerLoad::FindOrCreate(package);
    std::string output;
    Runtime::FJsonWriter writer(output);
    writer.WriteArrayStart();
    for (std::size_t index = 0; index < package.getExportCount(); index++)
        if (const Runtime::UObject* object = linker->CreateExport(static_cast<Runtime::int32>(index)))
            Runtime::FJsonObjectConverter::UObjectToJsonObject(object, writer);
    writer.WriteArrayEnd();
    output += '\n';
    std::fwrite(output.data(), 1, output.size(), stdout);
    return 0;
}
