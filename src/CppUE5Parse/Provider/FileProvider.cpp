#include <mutex>

#include <ankerl/unordered_dense.h>
#include <oneapi/tbb/collaborative_call_once.h>

#include "CppUE5Parse/Package/Package.h"
#include "CppUE5Parse/Provider/FileProvider.h"
#include "CppUE5Parse/Registry/AssetRegistry.h"

namespace ue5parse {

namespace {

struct PackageSlot {
    tbb::collaborative_once_flag once;
    Result<std::unique_ptr<Package>> result;
};

struct RegistrySlot {
    tbb::collaborative_once_flag once;
    Result<FAssetRegistryState> result;
};

std::string_view getPackageName(std::string_view path) {
    std::size_t slash = path.rfind('/');
    std::size_t dot = path.find('.', slash == std::string_view::npos ? 0 : slash);
    std::size_t colon = path.find(':', slash == std::string_view::npos ? 0 : slash);
    std::size_t end = dot < colon ? dot : colon;
    return end == std::string_view::npos ? path : path.substr(0, end);
}

}

struct FileProvider::Cache {
    std::unique_ptr<PackageSlot[]> packages;
    std::mutex registriesMutex;
    ankerl::unordered_dense::map<std::string, std::unique_ptr<RegistrySlot>> registries;
};

Result<MountedGame> mount(MountOptions options, std::stop_token stop) {
    std::unique_ptr<Codecs> codecs = options.codecs != nullptr ? std::move(options.codecs) : std::make_unique<Codecs>();
    Mappings mappings;
    if (options.mappings != nullptr) {
        Result<Mappings> parsed = readMappings(*options.mappings, *codecs);
        if (!parsed) return std::unexpected(parsed.error());
        mappings = std::move(*parsed);
    }
    Result<ArchiveSet> set = readArchives(std::move(options.archives), std::move(options.decryptor), std::move(codecs), stop);
    if (!set) return std::unexpected(set.error());
    FileSystem files(std::move(*set));
    if (stop.stop_requested()) return fail(ErrorCode::Cancelled, "mount");
    Result<ScriptObjects> scripts = readScriptObjects(files, stop);
    if (!scripts) return std::unexpected(scripts.error());
    Result<PackageStore> store = readPackageStore(files, stop);
    if (!store) return std::unexpected(store.error());
    return MountedGame{std::move(files), std::move(mappings), std::move(*scripts), std::move(*store), options.game};
}

FileProvider::FileProvider(MountedGame game) : game(std::move(game)), cache(std::make_unique<Cache>()) {
    cache->packages = std::make_unique<PackageSlot[]>(this->game.packageStore.getEntryCount());
}

FileProvider::~FileProvider() = default;

Result<const Package*> FileProvider::loadPackage(std::string_view path, std::stop_token stop) const {
    return loadPackage(FPackageId::fromName(getPackageName(path)), stop);
}

Result<const Package*> FileProvider::loadPackage(FPackageId id, std::stop_token stop) const {
    if (stop.stop_requested()) return fail(ErrorCode::Cancelled, "FileProvider.loadPackage");
    std::optional<ChunkLocation> location = game.files.findChunk(FIoChunkId::create(id.value, 0, EIoChunkType::ExportBundleData));
    if (!location) return fail(ErrorCode::MissingPackage, "FileProvider.loadPackage");
    std::optional<std::uint32_t> index = game.packageStore.findIndex(location->archive, id);
    if (!index) return fail(ErrorCode::MissingPackage, "FileProvider.loadPackage.store");
    PackageSlot& slot = cache->packages[*index];
    tbb::collaborative_call_once(slot.once, [&] { slot.result = readPackage(id, *location, game.packageStore.getEntry(*index)); });
    if (!slot.result) return std::unexpected(slot.result.error());
    return slot.result->get();
}

Result<std::unique_ptr<Package>> FileProvider::readPackage(FPackageId id, ChunkLocation location, const PackageStoreEntry& entry) const {
    std::uint64_t size = game.files.getChunkSize(location);
    std::vector<std::byte> bytes(size < 65536 ? size : 65536);
    Result<void> read = game.files.readChunk(location, 0, bytes);
    if (!read) return std::unexpected(read.error());
    std::uint32_t headerSize = peekZenHeaderSize(bytes);
    if (headerSize == 0 || headerSize > size) return fail(ErrorCode::InvalidData, "FileProvider.loadPackage.headerSize");
    if (headerSize > bytes.size()) {
        bytes.resize(headerSize);
        read = game.files.readChunk(location, 0, bytes);
        if (!read) return std::unexpected(read.error());
    }
    bytes.resize(headerSize);
    bytes.shrink_to_fit();
    Result<ZenPackageHeader> header = parseZenPackageHeader(std::move(bytes), game.game, &entry);
    if (!header) return std::unexpected(header.error());
    return std::make_unique<Package>(*this, location, id, std::move(*header));
}

Result<std::vector<std::byte>> FileProvider::readFile(std::string_view path, std::stop_token stop) const {
    std::optional<FileEntry> file = game.files.findFile(path);
    if (!file) return fail(ErrorCode::MissingFile, "FileProvider.readFile");
    std::vector<std::byte> bytes(game.files.getFileSize(*file));
    Result<void> read = game.files.readFile(*file, 0, bytes, stop);
    if (!read) return std::unexpected(read.error());
    return bytes;
}

Result<const FAssetRegistryState*> FileProvider::getAssetRegistry(std::string_view path) const {
    RegistrySlot* slot = nullptr;
    {
        std::lock_guard lock(cache->registriesMutex);
        std::unique_ptr<RegistrySlot>& found = cache->registries[std::string(path)];
        if (found == nullptr) found = std::make_unique<RegistrySlot>();
        slot = found.get();
    }
    tbb::collaborative_call_once(slot->once, [&] {
        Result<std::vector<std::byte>> bytes = readFile(path);
        slot->result = bytes ? parseAssetRegistry(*bytes) : std::unexpected(bytes.error());
    });
    if (!slot->result) return std::unexpected(slot->result.error());
    return &*slot->result;
}

}
