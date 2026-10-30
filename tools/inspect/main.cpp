// ue5inspect mappings <file.usmap> [<struct>]
// ue5inspect packages <Paks folder> <engine 5.0-5.8> [--key 0x...] [<package path>]
//
// Checks each layer of the library on real game data:
//   mappings   parses a .usmap, prints its counts, and the full property list of one struct
//   packages   reads the script objects and container headers, parses the header of every package, and prints one
// ue5inspect exports <Paks folder> <engine> <file.usmap> [--key 0x...] [--class <name>]
//   exports    reads the unversioned properties of every export of a native class and checks where they end

#include <tbb/parallel_for.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string_view>

#include "ue5parse/io/FileSource.h"
#include "ue5parse/package/PackageStore.h"
#include "ue5parse/package/ZenPackage.h"
#include "ue5parse/schema/Mappings.h"
#include "ue5parse/uobject/Properties.h"
#include "ue5parse/vfs/FileSystem.h"

#include <map>
#include <string>

using namespace ue5parse;

namespace {

void printError(std::string_view what, const Error& error) {
    std::printf("%.*s: %s at %s, offset %llu\n", static_cast<int>(what.size()), what.data(), getErrorName(error.code),
                error.where, static_cast<unsigned long long>(error.offset));
}

const char* getTypeName(EPropertyType type) {
    static const char* const names[] = {
        "Byte", "Bool", "Int", "Float", "Object", "Name", "Delegate", "Double", "Array", "Struct", "Str", "Text",
        "Interface", "MulticastDelegate", "WeakObject", "LazyObject", "AssetObject", "SoftObject", "UInt64", "UInt32",
        "UInt16", "Int64", "Int16", "Int8", "Map", "Set", "Enum", "FieldPath", "Optional", "Utf8Str", "AnsiStr", "Class",
        "MulticastInlineDelegate", "SoftClass", "VerseString", "VerseDynamic", "VerseFunction"};
    std::size_t index = static_cast<std::size_t>(type);
    return index < sizeof(names) / sizeof(names[0]) ? names[index] : "Unknown";
}

void printType(const PropertyType& type) {
    std::printf("%s", getTypeName(type.type));
    if (type.type == EPropertyType::StructProperty) {
        std::printf("<%.*s>", static_cast<int>(type.structName.getText().size()), type.structName.getText().data());
    } else if (type.type == EPropertyType::EnumProperty) {
        std::printf("<%.*s>", static_cast<int>(type.enumName.getText().size()), type.enumName.getText().data());
    } else if (type.inner != nullptr) {
        std::printf("<");
        printType(*type.inner);
        if (type.value != nullptr) {
            std::printf(", ");
            printType(*type.value);
        }
        std::printf(">");
    }
}

int inspectMappings(const char* path, const char* structName) {
    Result<std::unique_ptr<FileSource>> file = openLocalFile(path);
    if (!file) {
        printError(path, file.error());
        return 1;
    }
    Codecs codecs;
    auto start = std::chrono::steady_clock::now();
    Result<Mappings> mappings = readMappings(**file, codecs);
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    if (!mappings) {
        printError(path, mappings.error());
        return 1;
    }
    std::size_t unresolvedSupers = 0;
    for (std::size_t i = 0; i < mappings->getStructCount(); i++) {
        const StructSchema& schema = mappings->getStruct(i);
        if (!schema.superName.isNone() && schema.super == nullptr) unresolvedSupers++;
    }
    std::printf("%zu structs, %zu enums in %.3f s; %zu supers not mapped\n", mappings->getStructCount(),
                mappings->getEnumCount(), seconds, unresolvedSupers);
    if (structName == nullptr) return 0;

    const StructSchema* schema = mappings->findStruct(structName);
    if (schema == nullptr) {
        std::printf("%s: not found\n", structName);
        return 1;
    }
    // The full index space, own properties first, as the unversioned format numbers them.
    std::uint32_t index = 0;
    for (const StructSchema* current = schema; current != nullptr; current = current->super) {
        std::printf("-- %.*s (%u)\n", static_cast<int>(current->name.getText().size()), current->name.getText().data(),
                    current->propertyCount);
        for (std::uint32_t i = 0; i < current->propertyCount; i++, index++) {
            const PropertyInfo* property = findProperty(*schema, index);
            if (property == nullptr) continue;
            std::printf("%5u %.*s", index, static_cast<int>(property->name.getText().size()), property->name.getText().data());
            if (property->arrayDim > 1) std::printf("[%u]", property->arrayIndex);
            std::printf(": ");
            printType(*property->type);
            std::printf("\n");
        }
    }
    return 0;
}

double secondsSince(std::chrono::steady_clock::time_point start) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

std::optional<FileSystem> mountFolder(const char* folder, const char* key) {
    std::vector<std::unique_ptr<FileSource>> sources;
    std::error_code error;
    for (const auto& entry : std::filesystem::directory_iterator(folder, error)) {
        if (!entry.is_regular_file()) continue;
        Result<std::unique_ptr<FileSource>> file = openLocalFile(entry.path());
        if (file) sources.push_back(std::move(*file));
    }
    std::unique_ptr<Decryptor> decryptor;
    if (key != nullptr) {
        Result<AesKey> parsed = parseAesKey(key);
        if (!parsed) {
            printError("--key", parsed.error());
            return std::nullopt;
        }
        KeyRing keys;
        keys.add(FGuid{}, *parsed);
        decryptor = std::make_unique<AesDecryptor>(keys);
    }
    Result<ArchiveSet> set = readArchives(std::move(sources), std::move(decryptor), nullptr);
    if (!set) {
        printError("mount", set.error());
        return std::nullopt;
    }
    return FileSystem(std::move(*set));
}

std::optional<EngineVersion> parseEngine(std::string_view text) {
    if (text.size() != 3 || text[0] != '5' || text[1] != '.' || text[2] < '0' || text[2] > '8') return std::nullopt;
    return static_cast<EngineVersion>(500 + (text[2] - '0'));
}

// The name of a package object index: an export or a script object, or the package and hash of an import.
void printObjectIndex(const ZenPackageHeader& header, const ScriptObjects& scripts, FPackageObjectIndex index) {
    if (index.isNull()) {
        std::printf("null");
    } else if (index.isExport()) {
        if (index.getIndex() >= header.getExportMap().size()) {
            std::printf("export %llu (bad)", static_cast<unsigned long long>(index.getIndex()));
            return;
        }
        FName name = header.getName(header.getExportMap()[index.getIndex()].objectName, scripts.getNames());
        std::printf("export %.*s", static_cast<int>(name.getText().size()), name.getText().data());
    } else if (index.isScriptImport()) {
        const FScriptObjectEntry* entry = scripts.find(index);
        if (entry == nullptr) {
            std::printf("script ? %016llx", static_cast<unsigned long long>(index.value));
            return;
        }
        FName name = scripts.getName(*entry);
        const FScriptObjectEntry* outer = scripts.getOuter(*entry);
        FName outerName = outer != nullptr ? scripts.getName(*outer) : FName();
        std::printf("%.*s.%.*s", static_cast<int>(outerName.getText().size()), outerName.getText().data(),
                    static_cast<int>(name.getText().size()), name.getText().data());
    } else {
        std::printf("import package %u hash %u", index.getImportedPackageIndex(), index.getImportedPublicExportHashIndex());
    }
}

int inspectPackages(const char* folder, EngineVersion engine, const char* key, const char* path) {
    auto start = std::chrono::steady_clock::now();
    std::optional<FileSystem> files = mountFolder(folder, key);
    if (!files) return 1;
    std::printf("mounted %zu archives in %.3f s\n", files->getArchiveCount(), secondsSince(start));

    start = std::chrono::steady_clock::now();
    Result<ScriptObjects> scripts = readScriptObjects(*files);
    if (!scripts) {
        printError("script objects", scripts.error());
        return 1;
    }
    Result<PackageStore> store = readPackageStore(*files);
    if (!store) {
        printError("package store", store.error());
        return 1;
    }
    for (const ArchiveProblem& problem : store->getProblems()) printError(problem.name, problem.error);
    std::printf("%zu script objects, %zu global names, %zu store entries in %.3f s\n", scripts->getCount(),
                scripts->getNames().size(), store->getEntryCount(), secondsSince(start));

    auto readHeader = [&](ChunkLocation location, FPackageId id) -> Result<ZenPackageHeader> {
        std::uint64_t chunkSize = files->getChunkSize(location);
        std::vector<std::byte> summary(std::min<std::uint64_t>(chunkSize, getZenSummarySize(engine)));
        Result<void> read = files->readChunk(location, 0, summary);
        if (!read) return std::unexpected(read.error());
        std::uint32_t headerSize = peekZenHeaderSize(summary);
        if (headerSize > chunkSize) return fail(ErrorCode::InvalidData, "headerSize");
        std::vector<std::byte> bytes(headerSize);
        read = files->readChunk(location, 0, bytes);
        if (!read) return std::unexpected(read.error());
        return parseZenPackageHeader(std::move(bytes), engine, store->find(location.archive, id));
    };

    if (path != nullptr) {
        FPackageId id = FPackageId::fromName(path);
        std::optional<ChunkLocation> location = files->findChunk(FIoChunkId::create(id.value, 0, EIoChunkType::ExportBundleData));
        if (!location) {
            std::printf("%s: no package chunk\n", path);
            return 1;
        }
        Result<ZenPackageHeader> header = readHeader(*location, id);
        if (!header) {
            printError(path, header.error());
            return 1;
        }
        FName name = header->getName();
        std::printf("%.*s: header %u bytes (cooked %u), %zu names, %zu imports, %zu exports, %zu imported packages, %zu bulk data\n",
                    static_cast<int>(name.getText().size()), name.getText().data(), header->getHeaderSize(),
                    header->getCookedHeaderSize(), header->getNameMap().size(), header->getImportMap().size(),
                    header->getExportMap().size(), header->getImportedPackages().size(), header->getBulkDataMap().size());
        for (std::size_t i = 0; i < header->getImportMap().size(); i++) {
            std::printf("  import %zu: ", i);
            printObjectIndex(*header, *scripts, header->getImportMap()[i]);
            std::printf("\n");
        }
        for (std::size_t i = 0; i < header->getExportMap().size(); i++) {
            const FExportMapEntry& entry = header->getExportMap()[i];
            FName exportName = header->getName(entry.objectName, scripts->getNames());
            std::printf("  export %zu: %.*s, class ", i, static_cast<int>(exportName.getText().size()), exportName.getText().data());
            printObjectIndex(*header, *scripts, entry.classIndex);
            std::printf(", outer ");
            printObjectIndex(*header, *scripts, entry.outerIndex);
            std::printf(", at %llu, %llu bytes\n", static_cast<unsigned long long>(header->getExportChunkOffset(i)),
                        static_cast<unsigned long long>(entry.cookedSerialSize));
        }
        return 0;
    }

    // Every package chunk of every archive.
    struct Package {
        ChunkLocation location;
        FPackageId id;
    };
    std::vector<Package> packages;
    for (std::size_t archive = 0; archive < files->getArchiveCount(); archive++) {
        const IoStoreContainer* container = files->getArchive(archive).ioStore.get();
        if (container == nullptr) continue;
        const IoStoreToc& toc = container->getToc();
        for (std::size_t i = 0; i < toc.getChunkCount(); i++) {
            const FIoChunkId& id = toc.getChunkId(i);
            if (id.getType() != EIoChunkType::ExportBundleData || id.getIndex() != 0) continue;
            packages.push_back({ChunkLocation{static_cast<std::uint32_t>(archive), static_cast<std::uint32_t>(i)}, FPackageId{id.getId()}});
        }
    }
    start = std::chrono::steady_clock::now();
    std::atomic<std::size_t> failures = 0, exports = 0, imports = 0, missingStore = 0, badOffsets = 0, unresolvedScript = 0;
    std::mutex printMutex;
    tbb::parallel_for(std::size_t(0), packages.size(), [&](std::size_t i) {
        Result<ZenPackageHeader> header = readHeader(packages[i].location, packages[i].id);
        if (!header) {
            if (failures++ < 20) {
                std::scoped_lock lock(printMutex);
                printError("package", header.error());
            }
            return;
        }
        if (store->find(packages[i].location.archive, packages[i].id) == nullptr) missingStore++;
        exports += header->getExportMap().size();
        imports += header->getImportMap().size();
        std::uint64_t chunkSize = files->getChunkSize(packages[i].location);
        for (std::size_t e = 0; e < header->getExportMap().size(); e++)
            if (header->getExportChunkOffset(e) + header->getExportMap()[e].cookedSerialSize > chunkSize) badOffsets++;
        for (FPackageObjectIndex index : header->getImportMap())
            if (index.isScriptImport() && scripts->find(index) == nullptr) unresolvedScript++;
    });
    std::printf("%zu packages, %zu exports, %zu imports in %.2f s; %zu failed, %zu without store entry, "
                "%zu exports outside their chunk, %zu unknown script imports\n",
                packages.size(), exports.load(), imports.load(), secondsSince(start), failures.load(), missingStore.load(),
                badOffsets.load(), unresolvedScript.load());
    return failures == 0 && badOffsets == 0 ? 0 : 1;
}

struct ExportStats {
    std::size_t exact = 0;      // the properties, then only the has-guid flag (and the guid)
    std::size_t tail = 0;       // more data after them: native serialization
    std::size_t cdo = 0;        // class default objects end after the properties
    std::size_t failed = 0;
    std::size_t blueprint = 0;  // the class is not native: not read yet
    std::size_t noSchema = 0;
};

int inspectExports(const char* folder, EngineVersion engine, const char* usmap, const char* key, const char* onlyClass) {
    std::optional<FileSystem> files = mountFolder(folder, key);
    if (!files) return 1;
    Codecs codecs;
    Result<std::unique_ptr<FileSource>> usmapFile = openLocalFile(usmap);
    if (!usmapFile) {
        printError(usmap, usmapFile.error());
        return 1;
    }
    Result<Mappings> mappings = readMappings(**usmapFile, codecs);
    Result<ScriptObjects> scripts = readScriptObjects(*files);
    Result<PackageStore> store = readPackageStore(*files);
    if (!mappings || !scripts || !store) {
        std::printf("setup failed\n");
        return 1;
    }

    std::vector<std::pair<ChunkLocation, FPackageId>> packages;
    for (std::size_t archive = 0; archive < files->getArchiveCount(); archive++) {
        const IoStoreContainer* container = files->getArchive(archive).ioStore.get();
        if (container == nullptr) continue;
        const IoStoreToc& toc = container->getToc();
        for (std::size_t i = 0; i < toc.getChunkCount(); i++) {
            const FIoChunkId& id = toc.getChunkId(i);
            if (id.getType() != EIoChunkType::ExportBundleData || id.getIndex() != 0) continue;
            packages.push_back({ChunkLocation{static_cast<std::uint32_t>(archive), static_cast<std::uint32_t>(i)}, FPackageId{id.getId()}});
        }
    }

    std::mutex mutex;
    ExportStats total;
    std::map<std::string, std::size_t> failuresByError;
    std::map<std::string, std::pair<std::size_t, std::size_t>> byClass;   // exact, tail
    std::vector<std::string> examples;
    std::uint64_t propertyCount = 0;
    auto start = std::chrono::steady_clock::now();
    tbb::parallel_for(std::size_t(0), packages.size(), [&](std::size_t p) {
        auto [location, id] = packages[p];
        std::vector<std::byte> chunk(files->getChunkSize(location));
        if (!files->readChunk(location, 0, chunk)) return;
        std::uint32_t headerSize = peekZenHeaderSize(chunk);
        if (headerSize > chunk.size()) return;
        Result<ZenPackageHeader> header = parseZenPackageHeader(std::vector<std::byte>(chunk.begin(), chunk.begin() + headerSize),
                                                                engine, store->find(location.archive, id));
        if (!header) return;
        PropertyContext context{&*mappings, header->getNameMap(), engine};
        ExportStats local;
        std::uint64_t localProperties = 0;
        std::vector<FPropertyTag> tags;
        for (std::size_t e = 0; e < header->getExportMap().size(); e++) {
            const FExportMapEntry& entry = header->getExportMap()[e];
            const FScriptObjectEntry* classEntry = scripts->find(entry.classIndex);
            if (classEntry == nullptr) {
                local.blueprint++;
                continue;
            }
            FName className = scripts->getName(*classEntry);
            if (onlyClass != nullptr && className.getText() != onlyClass) continue;
            const StructSchema* schema = mappings->findStruct(className);
            if (schema == nullptr) {
                local.noSchema++;
                continue;
            }
            std::uint64_t begin = header->getExportChunkOffset(e);
            if (begin + entry.cookedSerialSize > chunk.size()) continue;
            ByteReader reader{std::span<const std::byte>(chunk).subspan(begin, entry.cookedSerialSize)};
            tags.clear();
            Result<void> read = readUnversionedProperties(reader, context, *schema, tags);
            if (!read) {
                local.failed++;
                std::scoped_lock lock(mutex);
                failuresByError[std::string(read.error().where) + " / " + std::string(className.getText())]++;
                if (examples.size() < 30) {
                    FName name = header->getName(entry.objectName, scripts->getNames());
                    examples.push_back(std::string(header->getName().getText()) + "." + std::string(name.getText()) + " (" +
                                       std::string(className.getText()) + "): " + read.error().where + " at " +
                                       std::to_string(read.error().offset) + " of " + std::to_string(entry.cookedSerialSize));
                }
                continue;
            }
            localProperties += tags.size();
            bool isCdo = (entry.objectFlags & 0x10) != 0;
            std::size_t remaining = reader.getRemaining();
            bool exact;
            if (isCdo) {
                local.cdo++;
                exact = remaining == 0;
            } else {
                std::int32_t hasGuid = remaining >= 4 ? reader.read<std::int32_t>() : -1;
                exact = (remaining == 4 && hasGuid == 0) || (remaining == 20 && hasGuid == 1);
            }
            if (exact) local.exact++;
            else local.tail++;
            if (!exact && onlyClass != nullptr) {
                std::scoped_lock lock(mutex);
                if (examples.size() < 12) {
                    std::string text = "remaining " + std::to_string(remaining) + ":";
                    std::span<const std::byte> rest = reader.getData().subspan(reader.getSize() - remaining);
                    for (std::size_t b = 0; b < rest.size() && b < 48; b++) {
                        char hex[4];
                        std::snprintf(hex, sizeof(hex), " %02x", static_cast<unsigned>(rest[b]));
                        text += hex;
                    }
                    examples.push_back(text);
                }
            }
            std::scoped_lock lock(mutex);
            auto& counts = byClass[std::string(className.getText())];
            (exact ? counts.first : counts.second)++;
        }
        std::scoped_lock lock(mutex);
        total.exact += local.exact;
        total.tail += local.tail;
        total.cdo += local.cdo;
        total.failed += local.failed;
        total.blueprint += local.blueprint;
        total.noSchema += local.noSchema;
        propertyCount += localProperties;
    });
    std::printf("%.2f s: %zu exact, %zu with native data after the properties, %zu failed (%zu CDOs), %zu of Blueprint classes, "
                "%zu without schema; %llu properties\n", secondsSince(start), total.exact, total.tail, total.failed, total.cdo,
                total.blueprint, total.noSchema, static_cast<unsigned long long>(propertyCount));
    std::vector<std::pair<std::size_t, std::string>> errors;
    for (const auto& [what, count] : failuresByError) errors.push_back({count, what});
    std::sort(errors.rbegin(), errors.rend());
    for (std::size_t i = 0; i < errors.size() && i < 25; i++) std::printf("  %6zu  %s\n", errors[i].first, errors[i].second.c_str());
    for (const std::string& example : examples) std::printf("  e.g. %s\n", example.c_str());
    // Classes that always have data after their properties (native Serialize), most common first.
    std::vector<std::pair<std::size_t, std::string>> tails;
    for (const auto& [name, counts] : byClass)
        if (counts.second > 0) tails.push_back({counts.second, name + " (" + std::to_string(counts.first) + " exact)"});
    std::sort(tails.rbegin(), tails.rend());
    std::printf("classes with data after the properties:\n");
    for (std::size_t i = 0; i < tails.size() && i < 40; i++) std::printf("  %6zu  %s\n", tails[i].first, tails[i].second.c_str());
    return 0;
}

// One export: each property read, with its type and byte range, then where reading stopped.
int inspectObject(const char* folder, EngineVersion engine, const char* usmap, const char* key, const char* packagePath,
                  const char* exportName) {
    std::optional<FileSystem> files = mountFolder(folder, key);
    if (!files) return 1;
    Codecs codecs;
    Result<std::unique_ptr<FileSource>> usmapFile = openLocalFile(usmap);
    if (!usmapFile) return 1;
    Result<Mappings> mappings = readMappings(**usmapFile, codecs);
    Result<ScriptObjects> scripts = readScriptObjects(*files);
    Result<PackageStore> store = readPackageStore(*files);
    if (!mappings || !scripts || !store) return 1;
    FPackageId id = FPackageId::fromName(packagePath);
    std::optional<ChunkLocation> location = files->findChunk(FIoChunkId::create(id.value, 0, EIoChunkType::ExportBundleData));
    if (!location) return 1;
    std::vector<std::byte> chunk(files->getChunkSize(*location));
    if (!files->readChunk(*location, 0, chunk)) return 1;
    std::uint32_t headerSize = peekZenHeaderSize(chunk);
    Result<ZenPackageHeader> header = parseZenPackageHeader(std::vector<std::byte>(chunk.begin(), chunk.begin() + headerSize),
                                                            engine, store->find(location->archive, id));
    if (!header) return 1;
    PropertyContext context{&*mappings, header->getNameMap(), engine};
    for (std::size_t e = 0; e < header->getExportMap().size(); e++) {
        const FExportMapEntry& entry = header->getExportMap()[e];
        FName name = header->getName(entry.objectName, scripts->getNames());
        if (name.getText() != exportName) continue;
        const FScriptObjectEntry* classEntry = scripts->find(entry.classIndex);
        if (classEntry == nullptr) {
            std::printf("not a native class\n");
            return 1;
        }
        const StructSchema* schema = mappings->findStruct(scripts->getName(*classEntry));
        ByteReader reader{std::span<const std::byte>(chunk).subspan(header->getExportChunkOffset(e), entry.cookedSerialSize)};
        std::vector<FPropertyTag> tags;
        Result<void> read = readUnversionedProperties(reader, context, *schema, tags);
        for (const FPropertyTag& tag : tags) {
            std::printf("%6u %5u %.*s", tag.offset, tag.size, static_cast<int>(tag.name.getText().size()), tag.name.getText().data());
            if (tag.arrayIndex > 0) std::printf("[%u]", tag.arrayIndex);
            std::printf(": ");
            printType(*tag.type);
            std::printf("%s\n", tag.zero ? " (zero)" : "");
        }
        if (!read) printError("stopped", read.error());
        else std::printf("end at %zu of %llu\n", reader.getPosition(), static_cast<unsigned long long>(entry.cookedSerialSize));
        return 0;
    }
    std::printf("no export %s\n", exportName);
    return 1;
}

}

int main(int argc, char** argv) {
    if (argc >= 3 && std::strcmp(argv[1], "mappings") == 0) return inspectMappings(argv[2], argc >= 4 ? argv[3] : nullptr);
    if (argc >= 4 && std::strcmp(argv[1], "packages") == 0) {
        std::optional<EngineVersion> engine = parseEngine(argv[3]);
        if (!engine) {
            std::fprintf(stderr, "engine must be 5.0 to 5.8\n");
            return 2;
        }
        const char* key = nullptr;
        const char* path = nullptr;
        for (int i = 4; i < argc; i++) {
            if (std::strcmp(argv[i], "--key") == 0 && i + 1 < argc) key = argv[++i];
            else path = argv[i];
        }
        return inspectPackages(argv[2], *engine, key, path);
    }
    if (argc >= 5 && std::strcmp(argv[1], "exports") == 0) {
        std::optional<EngineVersion> engine = parseEngine(argv[3]);
        if (!engine) return 2;
        const char* key = nullptr;
        const char* onlyClass = nullptr;
        for (int i = 5; i < argc; i++) {
            if (std::strcmp(argv[i], "--key") == 0 && i + 1 < argc) key = argv[++i];
            else if (std::strcmp(argv[i], "--class") == 0 && i + 1 < argc) onlyClass = argv[++i];
        }
        return inspectExports(argv[2], *engine, argv[4], key, onlyClass);
    }
    if (argc >= 7 && std::strcmp(argv[1], "object") == 0) {
        std::optional<EngineVersion> engine = parseEngine(argv[3]);
        if (!engine) return 2;
        const char* key = nullptr;
        for (int i = 7; i < argc; i++)
            if (std::strcmp(argv[i], "--key") == 0 && i + 1 < argc) key = argv[++i];
        return inspectObject(argv[2], *engine, argv[4], key, argv[5], argv[6]);
    }
    std::fprintf(stderr, "usage: ue5inspect mappings <file.usmap> [<struct>]\n"
                         "       ue5inspect packages <Paks folder> <engine 5.0-5.8> [--key 0x...] [<package path>]\n");
    return 2;
}
